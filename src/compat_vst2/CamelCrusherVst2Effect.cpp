#include "CamelCrusherVst2Effect.h"

#include "CamelCrusherVst2Editor.h"
#include "DefaultLegacyFactoryBank.h"

#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyParameters.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <span>
#include <string>
#include <system_error>

namespace camelcrusher_recalled {
namespace {

constexpr std::string_view kEffectName = "CamelCrusher";
constexpr std::string_view kVendorName = "Camel Audio";
constexpr VstInt32 kLegacyPluginVersion = 101;
constexpr VstInt32 kMonoChannelCount = 1;
constexpr VstInt32 kStereoChannelCount = 2;
constexpr int kMaxTraceProcessEvents = 512;

std::mutex& traceMutex() {
  static std::mutex mutex;
  return mutex;
}

std::filesystem::path traceLogPath() {
  if (const char* home = std::getenv("HOME")) {
    return std::filesystem::path(home) / "Library" / "Logs" /
           "CamelCrusherRebuilt" / "vst2-runtime.log";
  }
  return std::filesystem::temp_directory_path() / "camelcrusher-vst2-runtime.log";
}

void appendTraceLine(const std::string& line) {
  std::lock_guard<std::mutex> lock(traceMutex());
  const auto path = traceLogPath();
  std::error_code error;
  std::filesystem::create_directories(path.parent_path(), error);
  std::ofstream stream(path, std::ios::app);
  if (!stream.is_open()) {
    return;
  }
  stream << line << '\n';
}

std::string formatSpeakerArrangement(const VstSpeakerArrangement* arrangement) {
  if (arrangement == nullptr) {
    return "null";
  }

  return "type=" + std::to_string(arrangement->type) +
         " channels=" + std::to_string(arrangement->numChannels);
}

struct ChannelStats {
  float first_sample = 0.0F;
  float mean_abs = 0.0F;
  float max_abs = 0.0F;
};

ChannelStats computeChannelStats(const float* buffer, const VstInt32 sample_frames) {
  ChannelStats stats;
  if (buffer == nullptr || sample_frames <= 0) {
    return stats;
  }

  stats.first_sample = buffer[0];
  const auto frame_count = std::min<VstInt32>(sample_frames, 256);
  float abs_sum = 0.0F;
  float abs_max = 0.0F;
  for (VstInt32 frame = 0; frame < frame_count; ++frame) {
    const auto value = std::abs(buffer[frame]);
    abs_sum += value;
    abs_max = std::max(abs_max, value);
  }

  stats.mean_abs = abs_sum / static_cast<float>(frame_count);
  stats.max_abs = abs_max;
  return stats;
}

bool isSwitchParameter(const VstInt32 index) {
  const auto parameter_id = legacyParameterIdFromLegacyIndex(index);
  return parameter_id.has_value() && isSwitchLikeParameter(parameter_id.value());
}

LegacyChunk bankToChunk(const LegacyPresetBank& preset_bank) {
  LegacyChunk chunk;
  chunk.programs.reserve(preset_bank.presets.size());

  for (const auto& preset : preset_bank.presets) {
    chunk.programs.push_back(LegacyProgram{
        .record_marker = preset.record_marker,
        .name = preset.name,
        .parameter_values = legacyStateToArray(preset.state),
    });
  }

  return chunk;
}

LegacyPreset makePreset(std::size_t program_index, const LegacyProgram& program) {
  return LegacyPreset{
      .program_index = program_index,
      .record_marker = program.record_marker,
      .name = program.name,
      .state = legacyStateFromArray(program.parameter_values),
  };
}

VstSpeakerArrangement makeSpeakerArrangement(const VstInt32 type,
                                            const VstInt32 num_channels) {
  VstSpeakerArrangement arrangement{};
  arrangement.type = type;
  arrangement.numChannels = num_channels;
  return arrangement;
}

bool isMonoArrangement(const VstSpeakerArrangement* arrangement) {
  return arrangement != nullptr &&
         arrangement->type == kSpeakerArrMono &&
         arrangement->numChannels == kMonoChannelCount;
}

bool isStereoArrangement(const VstSpeakerArrangement* arrangement) {
  return arrangement != nullptr &&
         arrangement->type == kSpeakerArrStereo &&
         arrangement->numChannels == kStereoChannelCount;
}

void initializePinProperties(VstPinProperties& properties,
                             const VstInt32 index,
                             const std::string_view prefix) {
  std::memset(&properties, 0, sizeof(VstPinProperties));
  properties.flags = kVstPinIsActive | kVstPinIsStereo;
  properties.arrangementType = kSpeakerArrStereo;

  const auto side = (index % 2) == 0 ? " L" : " R";
  std::snprintf(properties.label, sizeof(properties.label), "%.*s%s",
                static_cast<int>(prefix.size()), prefix.data(), side);
  std::snprintf(properties.shortLabel, sizeof(properties.shortLabel), "%s%s",
                prefix == "Input" ? "In" : "Out", side);
}

}  // namespace

CamelCrusherVst2Effect::CamelCrusherVst2Effect(
    const audioMasterCallback audio_master)
    : AudioEffectX(audio_master, kLegacyProgramCount, kLegacyParameterCount),
      preset_bank_(defaultProgramBank()),
      input_arrangement_(
          makeSpeakerArrangement(kSpeakerArrStereo, kStereoChannelCount)),
      output_arrangement_(
          makeSpeakerArrangement(kSpeakerArrStereo, kStereoChannelCount)) {
  trace_instance_id_ = nextTraceInstanceId();
  setEditor(createCamelCrusherVst2Editor(*this));
  setNumInputs(2);
  setNumOutputs(2);
  setUniqueID(kLegacyUniqueId);
  canProcessReplacing();
  programsAreChunks();
  adoptProgram(0);
  refreshProcessingState();
  appendTraceLine("instance=" + std::to_string(trace_instance_id_) +
                  " ctor flags=" + std::to_string(getAeffect()->flags));
}

void CamelCrusherVst2Effect::setSampleRate(const float sample_rate) {
  AudioEffectX::setSampleRate(sample_rate);
  prepareProcessor();
}

void CamelCrusherVst2Effect::setBlockSize(const VstInt32 block_size) {
  AudioEffectX::setBlockSize(block_size);
  prepareProcessor();
}

void CamelCrusherVst2Effect::resume() {
  AudioEffectX::resume();
  prepareProcessor();
}

void CamelCrusherVst2Effect::suspend() {
  processor_.reset();
  appendTraceLine("instance=" + std::to_string(trace_instance_id_) + " suspend");
}

void CamelCrusherVst2Effect::DECLARE_VST_DEPRECATED(process)(
    float** inputs,
    float** outputs,
    const VstInt32 sample_frames) {
  processFloatBuffers(inputs, outputs, sample_frames, "process");
}

void CamelCrusherVst2Effect::processReplacing(float** inputs,
                                              float** outputs,
                                              const VstInt32 sample_frames) {
  processFloatBuffers(inputs, outputs, sample_frames, "processReplacing");
}

void CamelCrusherVst2Effect::processFloatBuffers(float** inputs,
                                                 float** outputs,
                                                 const VstInt32 sample_frames,
                                                 const std::string_view callsite) {
  const auto right_input_connected =
      DECLARE_VST_DEPRECATED(isInputConnected)(1) ? 1 : 0;
  const auto right_output_connected =
      DECLARE_VST_DEPRECATED(isOutputConnected)(1) ? 1 : 0;
  const auto left_input_stats =
      computeChannelStats(inputs != nullptr ? inputs[0] : nullptr, sample_frames);
  const auto right_input_stats =
      computeChannelStats(inputs != nullptr ? inputs[1] : nullptr, sample_frames);
  const auto has_signal = left_input_stats.max_abs > 1.0e-6F ||
                          right_input_stats.max_abs > 1.0e-6F;

  if (trace_process_events_ < kMaxTraceProcessEvents || has_signal) {
    ++trace_process_events_;
    appendTraceLine(
        "instance=" + std::to_string(trace_instance_id_) +
        " " + std::string(callsite) +
        " call=" + std::to_string(trace_process_events_) +
        " frames=" + std::to_string(sample_frames) +
        " in0=" + std::to_string(inputs != nullptr && inputs[0] != nullptr) +
        " in1=" + std::to_string(inputs != nullptr && inputs[1] != nullptr) +
        " out0=" + std::to_string(outputs != nullptr && outputs[0] != nullptr) +
        " out1=" + std::to_string(outputs != nullptr && outputs[1] != nullptr) +
        " in1Connected=" + std::to_string(right_input_connected) +
        " out1Connected=" + std::to_string(right_output_connected) +
        " inPtrsSame=" + std::to_string(inputs != nullptr && inputs[0] == inputs[1]) +
        " outPtrsSame=" + std::to_string(outputs != nullptr && outputs[0] == outputs[1]) +
        " inArr=" + std::to_string(input_arrangement_.type) + "/" +
        std::to_string(input_arrangement_.numChannels) +
        " outArr=" + std::to_string(output_arrangement_.type) + "/" +
        std::to_string(output_arrangement_.numChannels) +
        " inL[first=" + std::to_string(left_input_stats.first_sample) +
        " meanAbs=" + std::to_string(left_input_stats.mean_abs) +
        " maxAbs=" + std::to_string(left_input_stats.max_abs) + "]" +
        " inR[first=" + std::to_string(right_input_stats.first_sample) +
        " meanAbs=" + std::to_string(right_input_stats.mean_abs) +
        " maxAbs=" + std::to_string(right_input_stats.max_abs) + "]");
  }

  if (outputs == nullptr || outputs[0] == nullptr || sample_frames <= 0) {
    return;
  }

  auto left_output = std::span<float>(outputs[0], sample_frames);
  auto right_output =
      outputs[1] != nullptr ? std::span<float>(outputs[1], sample_frames)
                            : std::span<float>{};

  const auto right_input_connected_runtime =
      DECLARE_VST_DEPRECATED(isInputConnected)(1);
  const auto host_left_only_mono_source =
      left_input_stats.max_abs > 1.0e-6F &&
      right_input_stats.max_abs <= 1.0e-6F &&
      input_arrangement_.type == kSpeakerArrStereo &&
      input_arrangement_.numChannels >= kStereoChannelCount &&
      right_input_connected_runtime;
  const auto mono_input_negotiated =
      input_arrangement_.type == kSpeakerArrMono ||
      input_arrangement_.numChannels <= kMonoChannelCount ||
      !right_input_connected_runtime ||
      host_left_only_mono_source;
  const float* left_input =
      inputs != nullptr && inputs[0] != nullptr ? inputs[0] : nullptr;
  const float* right_input = mono_input_negotiated
                                 ? left_input
                                 : (inputs != nullptr && inputs[1] != nullptr
                                        ? inputs[1]
                                        : left_input);

  if (trace_process_events_ <= kMaxTraceProcessEvents || has_signal) {
    appendTraceLine("instance=" + std::to_string(trace_instance_id_) +
                    " " + std::string(callsite) +
                    " hostLeftOnlyMono=" +
                    std::to_string(host_left_only_mono_source ? 1 : 0));
  }

  for (VstInt32 frame = 0; frame < sample_frames; ++frame) {
    const auto left_sample = left_input != nullptr ? left_input[frame] : 0.0F;
    left_output[frame] = left_sample;
    if (!right_output.empty()) {
      right_output[frame] =
          right_input != nullptr ? right_input[frame] : left_sample;
    }
  }

  processor_.processBlock(ModernAudioBlock{
                              .left = left_output,
                              .right = right_output,
                          },
                          processing_state_);

  if (trace_process_events_ <= kMaxTraceProcessEvents || has_signal) {
    const auto left_output_stats = computeChannelStats(outputs[0], sample_frames);
    const auto right_output_stats =
        computeChannelStats(outputs[1], sample_frames);
    appendTraceLine(
        "instance=" + std::to_string(trace_instance_id_) +
        " " + std::string(callsite) + " outL[first=" +
        std::to_string(left_output_stats.first_sample) + " meanAbs=" +
        std::to_string(left_output_stats.mean_abs) + " maxAbs=" +
        std::to_string(left_output_stats.max_abs) + "]" +
        " outR[first=" + std::to_string(right_output_stats.first_sample) +
        " meanAbs=" + std::to_string(right_output_stats.mean_abs) +
        " maxAbs=" + std::to_string(right_output_stats.max_abs) + "]");
  }
}

void CamelCrusherVst2Effect::setParameter(const VstInt32 index, const float value) {
  const auto parameter_id = legacyParameterIdFromLegacyIndex(index);
  if (!parameter_id.has_value()) {
    return;
  }

  setLegacyStateValue(current_state_, parameter_id.value(), clampNormalized(value));
  syncCurrentProgramState();
  refreshProcessingState();
}

float CamelCrusherVst2Effect::getParameter(const VstInt32 index) {
  const auto parameter_id = legacyParameterIdFromLegacyIndex(index);
  if (!parameter_id.has_value()) {
    return 0.0F;
  }

  return clampNormalized(legacyStateValue(current_state_, parameter_id.value()));
}

void CamelCrusherVst2Effect::setProgram(const VstInt32 program) {
  if (!isValidProgramIndex(program)) {
    return;
  }

  curProgram = program;
  adoptProgram(program);
}

void CamelCrusherVst2Effect::setProgramName(char* name) {
  if (!isValidProgramIndex(curProgram) || name == nullptr) {
    return;
  }

  preset_bank_.presets[static_cast<std::size_t>(curProgram)].name = name;
  refreshProcessingState();
}

void CamelCrusherVst2Effect::getProgramName(char* name) {
  if (name == nullptr || !isValidProgramIndex(curProgram)) {
    return;
  }

  copyVstString(name, kVstMaxProgNameLen,
                preset_bank_.presets[static_cast<std::size_t>(curProgram)].name);
}

bool CamelCrusherVst2Effect::getProgramNameIndexed(const VstInt32 /*category*/,
                                                   const VstInt32 index,
                                                   char* text) {
  if (text == nullptr || !isValidProgramIndex(index)) {
    return false;
  }

  copyVstString(text, kVstMaxProgNameLen,
                preset_bank_.presets[static_cast<std::size_t>(index)].name);
  return true;
}

void CamelCrusherVst2Effect::getParameterLabel(const VstInt32 index, char* label) {
  if (label == nullptr) {
    return;
  }

  copyVstString(label, kVstMaxParamStrLen, isSwitchParameter(index) ? "" : "%");
}

void CamelCrusherVst2Effect::getParameterDisplay(const VstInt32 index, char* text) {
  if (text == nullptr) {
    return;
  }

  if (isSwitchParameter(index)) {
    copyVstString(text, kVstMaxParamStrLen, getParameter(index) >= 0.5F ? "On" : "Off");
    return;
  }

  const auto percent = static_cast<int>(std::lround(getParameter(index) * 100.0F));
  std::snprintf(text, kVstMaxParamStrLen, "%d", percent);
}

void CamelCrusherVst2Effect::getParameterName(const VstInt32 index, char* text) {
  if (text == nullptr) {
    return;
  }

  const auto parameter_id = legacyParameterIdFromLegacyIndex(index);
  if (!parameter_id.has_value()) {
    copyVstString(text, kVstMaxParamStrLen, "");
    return;
  }

  copyVstString(text, kVstMaxParamStrLen,
                legacyParameterDescriptor(parameter_id.value()).display_name);
}

VstInt32 CamelCrusherVst2Effect::getChunk(void** data, const bool is_preset) {
  if (data == nullptr) {
    return 0;
  }

  if (is_preset && isValidProgramIndex(curProgram)) {
    syncCurrentProgramState();
    chunk_storage_ = encodePresetChunk(
        preset_bank_.presets[static_cast<std::size_t>(curProgram)]);
  } else {
    syncCurrentProgramState();
    chunk_storage_ = encodeBankChunk(preset_bank_);
  }

  *data = chunk_storage_.data();
  return static_cast<VstInt32>(chunk_storage_.size());
}

VstInt32 CamelCrusherVst2Effect::setChunk(void* data,
                                          const VstInt32 byte_size,
                                          const bool is_preset) {
  if (data == nullptr || byte_size <= 0) {
    return 0;
  }

  const auto* bytes = static_cast<const std::byte*>(data);
  const auto decoded = decodeLegacyChunk(std::span(bytes, static_cast<std::size_t>(byte_size)));
  if (!decoded.ok() || !decoded.chunk.has_value()) {
    return 0;
  }

  const auto& chunk = decoded.chunk.value();
  if (chunk.programs.empty()) {
    return 0;
  }

  if (is_preset || chunk.programs.size() == 1U) {
    if (!isValidProgramIndex(curProgram)) {
      curProgram = 0;
    }
    preset_bank_.presets[static_cast<std::size_t>(curProgram)] =
        makePreset(static_cast<std::size_t>(curProgram), chunk.programs.front());
    adoptProgram(curProgram);
    refreshProcessingState();
    return 1;
  }

  preset_bank_ = normalizedBank(legacyPresetBankFromChunk(chunk));
  if (!isValidProgramIndex(curProgram)) {
    curProgram = 0;
  } else if (static_cast<std::size_t>(curProgram) >= preset_bank_.presets.size()) {
    curProgram = static_cast<VstInt32>(preset_bank_.presets.size() - 1U);
  }
  adoptProgram(curProgram);
  refreshProcessingState();
  return 1;
}

bool CamelCrusherVst2Effect::getEffectName(char* name) {
  if (name == nullptr) {
    return false;
  }
  copyVstString(name, kVstMaxEffectNameLen, kEffectName);
  return true;
}

bool CamelCrusherVst2Effect::getVendorString(char* text) {
  if (text == nullptr) {
    return false;
  }
  copyVstString(text, kVstMaxVendorStrLen, kVendorName);
  return true;
}

bool CamelCrusherVst2Effect::getProductString(char* text) {
  if (text == nullptr) {
    return false;
  }
  copyVstString(text, kVstMaxProductStrLen, kEffectName);
  return true;
}

VstInt32 CamelCrusherVst2Effect::getVendorVersion() {
  return kLegacyPluginVersion;
}

VstInt32 CamelCrusherVst2Effect::getVstVersion() {
  return 2400;
}

VstPlugCategory CamelCrusherVst2Effect::getPlugCategory() {
  return kPlugCategUnknown;
}

VstInt32 CamelCrusherVst2Effect::canDo(char* text) {
  if (text == nullptr) {
    return -1;
  }

  constexpr std::array<std::string_view, 5> kSupportedCanDos{{
      "plugAsChannelInsert",
      "plugAsSend",
      "1in2out",
      "x2in2out",
      "bypass",
  }};

  const std::string_view query(text);
  return std::find(kSupportedCanDos.begin(), kSupportedCanDos.end(), query) !=
                 kSupportedCanDos.end()
             ? 1
             : -1;
}

bool CamelCrusherVst2Effect::getInputProperties(const VstInt32 index,
                                                VstPinProperties* properties) {
  if (properties == nullptr || index < 0 || index >= 2) {
    return false;
  }

  initializePinProperties(*properties, index, "Input");
  return true;
}

bool CamelCrusherVst2Effect::getOutputProperties(const VstInt32 index,
                                                 VstPinProperties* properties) {
  if (properties == nullptr || index < 0 || index >= 2) {
    return false;
  }

  initializePinProperties(*properties, index, "Output");
  return true;
}

bool CamelCrusherVst2Effect::setSpeakerArrangement(
    VstSpeakerArrangement* pluginInput,
    VstSpeakerArrangement* pluginOutput) {
  appendTraceLine("instance=" + std::to_string(trace_instance_id_) +
                  " setSpeakerArrangement in{" +
                  formatSpeakerArrangement(pluginInput) + "} out{" +
                  formatSpeakerArrangement(pluginOutput) + "}");
  if (pluginOutput != nullptr && !isStereoArrangement(pluginOutput)) {
    return false;
  }

  if (pluginInput != nullptr && !isMonoArrangement(pluginInput) &&
      !isStereoArrangement(pluginInput)) {
    return false;
  }

  input_arrangement_ = isMonoArrangement(pluginInput)
                           ? makeSpeakerArrangement(kSpeakerArrMono,
                                                    kMonoChannelCount)
                           : makeSpeakerArrangement(kSpeakerArrStereo,
                                                    kStereoChannelCount);
  output_arrangement_ =
      makeSpeakerArrangement(kSpeakerArrStereo, kStereoChannelCount);
  return true;
}

bool CamelCrusherVst2Effect::getSpeakerArrangement(
    VstSpeakerArrangement** pluginInput,
    VstSpeakerArrangement** pluginOutput) {
  if (pluginInput == nullptr || pluginOutput == nullptr) {
    return false;
  }

  *pluginInput = &input_arrangement_;
  *pluginOutput = &output_arrangement_;
  appendTraceLine("instance=" + std::to_string(trace_instance_id_) +
                  " getSpeakerArrangement in{" +
                  formatSpeakerArrangement(*pluginInput) + "} out{" +
                  formatSpeakerArrangement(*pluginOutput) + "}");
  return true;
}

void CamelCrusherVst2Effect::prepareProcessor() {
  processor_.prepare(ModernProcessSpec{
      .sample_rate = static_cast<double>(getSampleRate()),
      .max_block_size = static_cast<std::size_t>(std::max<VstInt32>(getBlockSize(), 1)),
      .channel_count = 2,
  });
}

void CamelCrusherVst2Effect::adoptProgram(const VstInt32 program) {
  if (!isValidProgramIndex(program)) {
    return;
  }

  const auto* preset = legacyPresetAt(preset_bank_, static_cast<std::size_t>(program));
  if (preset == nullptr) {
    return;
  }

  current_state_ = preset->state;
  refreshProcessingState();
  processor_.reset();
}

void CamelCrusherVst2Effect::syncCurrentProgramState() {
  if (!isValidProgramIndex(curProgram)) {
    return;
  }

  auto& preset = preset_bank_.presets[static_cast<std::size_t>(curProgram)];
  preset.program_index = static_cast<std::size_t>(curProgram);
  preset.state = current_state_;
}

void CamelCrusherVst2Effect::refreshProcessingState() {
  const auto* preset =
      isValidProgramIndex(curProgram)
          ? legacyPresetAt(preset_bank_, static_cast<std::size_t>(curProgram))
          : nullptr;

  processing_state_ = ModernPluginState{
      .state = current_state_,
      .legacy_preset_bank = preset_bank_,
      .selected_legacy_program_index =
          preset != nullptr
              ? std::optional<std::size_t>(static_cast<std::size_t>(curProgram))
              : std::nullopt,
      .selected_legacy_program_name = preset != nullptr ? preset->name : std::string{},
      .legacy_parameter_mismatches = {},
  };
}

LegacyPresetBank CamelCrusherVst2Effect::normalizedBank(LegacyPresetBank bank) {
  LegacyPresetBank normalized = defaultLegacyFactoryBank();
  if (normalized.presets.empty()) {
    normalized.presets.resize(kLegacyProgramCount);
  }
  if (normalized.presets.size() < static_cast<std::size_t>(kLegacyProgramCount)) {
    normalized.presets.resize(kLegacyProgramCount);
  } else if (normalized.presets.size() > static_cast<std::size_t>(kLegacyProgramCount)) {
    normalized.presets.resize(kLegacyProgramCount);
  }
  const auto copy_count =
      std::min(normalized.presets.size(), bank.presets.size());
  for (std::size_t index = 0; index < copy_count; ++index) {
    normalized.presets[index] = bank.presets[index];
  }

  for (std::size_t index = 0; index < normalized.presets.size(); ++index) {
    normalized.presets[index].program_index = index;
    if (normalized.presets[index].name.empty()) {
      normalized.presets[index].name = "Program " + std::to_string(index + 1U);
    }
  }

  return normalized;
}

LegacyPresetBank CamelCrusherVst2Effect::defaultProgramBank() {
  return normalizedBank({});
}

std::vector<std::byte> CamelCrusherVst2Effect::encodeBankChunk(
    const LegacyPresetBank& bank) {
  return encodeLegacyChunk(bankToChunk(bank));
}

std::vector<std::byte> CamelCrusherVst2Effect::encodePresetChunk(
    const LegacyPreset& preset) {
  LegacyChunk chunk;
  chunk.programs.push_back(LegacyProgram{
      .record_marker = preset.record_marker,
      .name = preset.name,
      .parameter_values = legacyStateToArray(preset.state),
  });
  return encodeLegacyChunk(chunk);
}

bool CamelCrusherVst2Effect::isValidProgramIndex(const VstInt32 program) {
  return program >= 0 && program < kLegacyProgramCount;
}

int CamelCrusherVst2Effect::nextTraceInstanceId() {
  static int next_id = 1;
  return next_id++;
}

void CamelCrusherVst2Effect::copyVstString(char* destination,
                                           const VstInt32 max_length,
                                           const std::string_view value) {
  if (destination == nullptr || max_length <= 0) {
    return;
  }

  const auto copy_length = std::min<std::size_t>(
      value.size(), static_cast<std::size_t>(max_length - 1));
  std::memcpy(destination, value.data(), copy_length);
  destination[copy_length] = '\0';
}

float CamelCrusherVst2Effect::clampNormalized(const float value) {
  return std::clamp(value, 0.0F, 1.0F);
}

AudioEffect* createCamelCrusherVst2Effect(const audioMasterCallback audio_master) {
  return new CamelCrusherVst2Effect(audio_master);
}

}  // namespace camelcrusher_recalled
