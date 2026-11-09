#include "camelcrusher_recalled/LegacyChunk.h"

#include "CamelCrusherVst2Effect.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <span>
#include <string_view>

namespace {

using camelcrusher_recalled::CamelCrusherVst2Effect;
using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::kLegacyProgramCount;
using camelcrusher_recalled::kLegacyUniqueId;

VstIntPtr hostCallback(AEffect* /*effect*/,
                       VstInt32 opcode,
                       VstInt32 /*index*/,
                       VstIntPtr /*value*/,
                       void* /*ptr*/,
                       float /*opt*/) {
  if (opcode == audioMasterVersion) {
    return 2400;
  }
  return 0;
}

void require(bool condition, const char* message) {
  if (!condition) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}

bool almostEqual(float lhs, float rhs) {
  return std::fabs(lhs - rhs) < 0.0001F;
}

}  // namespace

int main() {
  CamelCrusherVst2Effect effect(hostCallback);
  AEffect* aeffect = effect.getAeffect();

  require(aeffect != nullptr, "Expected a valid AEffect");
  require(aeffect->uniqueID == kLegacyUniqueId, "Expected legacy CamelCrusher unique ID");
  require(aeffect->numPrograms == kLegacyProgramCount, "Expected 20 programs");
  require(aeffect->numParams == 17, "Expected 17 parameters");
  require(aeffect->version == 1, "Expected legacy plugin version 1");
  require(aeffect->flags == 49,
          "Expected VST2 flags without legacy CanMono routing");
  require((aeffect->flags & effFlagsProgramChunks) != 0,
          "Expected chunk-based VST2 state");
  require((aeffect->flags & effFlagsCanMono) == 0,
          "Expected CanMono to be disabled for stable stereo routing");
  require(effect.getPlugCategory() == kPlugCategUnknown,
          "Expected original CamelCrusher plug-in category");

  VstPinProperties input_0{};
  VstPinProperties input_1{};
  VstPinProperties output_0{};
  VstPinProperties output_1{};
  require(effect.getInputProperties(0, &input_0),
          "Expected first VST2 input pin properties");
  require(effect.getInputProperties(1, &input_1),
          "Expected second VST2 input pin properties");
  require(effect.getOutputProperties(0, &output_0),
          "Expected first VST2 output pin properties");
  require(effect.getOutputProperties(1, &output_1),
          "Expected second VST2 output pin properties");

  const auto require_stereo_pin = [](const VstPinProperties& properties, bool first,
                                     const char* message) {
    require(properties.flags == (kVstPinIsActive | kVstPinUseSpeaker |
                                  (first ? kVstPinIsStereo : 0)),
            message);
    require(properties.arrangementType == kSpeakerArrStereo,
            "Expected stereo pin arrangement");
  };

  require_stereo_pin(input_0, true, "Expected first input pin to begin stereo pair");
  require_stereo_pin(input_1, false, "Expected second input pin to end stereo pair");
  require_stereo_pin(output_0, true, "Expected first output pin to begin stereo pair");
  require_stereo_pin(output_1, false, "Expected second output pin to end stereo pair");

  VstSpeakerArrangement mono_input{};
  mono_input.type = kSpeakerArrMono;
  mono_input.numChannels = 1;
  VstSpeakerArrangement stereo_input{};
  stereo_input.type = kSpeakerArrStereo;
  stereo_input.numChannels = 2;
  VstSpeakerArrangement stereo_output{};
  stereo_output.type = kSpeakerArrStereo;
  stereo_output.numChannels = 2;
  VstSpeakerArrangement mono_output{};
  mono_output.type = kSpeakerArrMono;
  mono_output.numChannels = 1;

  require(effect.setSpeakerArrangement(&mono_input, &stereo_output),
          "Expected mono input with stereo output to be accepted");
  VstSpeakerArrangement* negotiated_input = nullptr;
  VstSpeakerArrangement* negotiated_output = nullptr;
  require(effect.getSpeakerArrangement(&negotiated_input, &negotiated_output),
          "Expected speaker arrangement query to succeed");
  require(negotiated_input != nullptr && negotiated_input->type == kSpeakerArrMono &&
              negotiated_input->numChannels == 1,
          "Expected mono input arrangement to persist");
  require(negotiated_output != nullptr &&
              negotiated_output->type == kSpeakerArrStereo &&
              negotiated_output->numChannels == 2,
          "Expected stereo output arrangement to persist");

  require(effect.setSpeakerArrangement(&stereo_input, &stereo_output),
          "Expected stereo input with stereo output to be accepted");
  require(!effect.setSpeakerArrangement(&stereo_input, &mono_output),
          "Expected mono output arrangement to be rejected");

  char effect_name[kVstMaxEffectNameLen] = {};
  require(effect.getEffectName(effect_name), "Expected effect name");
  require(std::string_view(effect_name) == "CamelCrusher",
          "Expected CamelCrusher effect name");

  char program_name[kVstMaxProgNameLen] = {};
  effect.setProgram(3);
  effect.getProgramName(program_name);
  require(std::string_view(program_name) == "British Clean",
          "Expected default legacy factory bank");

  effect.setParameter(1, 0.42F);
  effect.setParameter(10, 0.9F);

  void* preset_data = nullptr;
  const auto preset_size = effect.getChunk(&preset_data, true);
  require(preset_size > 0, "Expected preset chunk bytes");
  const auto preset_chunk =
      decodeLegacyChunk(std::span(static_cast<const std::byte*>(preset_data),
                                  static_cast<std::size_t>(preset_size)));
  require(preset_chunk.ok(), "Expected preset chunk to decode");
  require(preset_chunk.chunk->programs.size() == 1U,
          "Expected single-program preset chunk");
  require(almostEqual(preset_chunk.chunk->programs.front().parameter_values[1], 0.42F),
          "Expected preset chunk to reflect current parameter values");

  void* bank_data = nullptr;
  const auto bank_size = effect.getChunk(&bank_data, false);
  require(bank_size > 0, "Expected bank chunk bytes");
  auto bank_chunk =
      decodeLegacyChunk(std::span(static_cast<const std::byte*>(bank_data),
                                  static_cast<std::size_t>(bank_size)));
  require(bank_chunk.ok(), "Expected bank chunk to decode");
  require(bank_chunk.chunk->programs.size() == 20U,
          "Expected 20-program legacy bank chunk");

  bank_chunk.chunk->programs[4].name = "Fixture Program";
  bank_chunk.chunk->programs[4].parameter_values[2] = 0.73F;
  auto mutated_bank_bytes = encodeLegacyChunk(bank_chunk.chunk.value());
  require(effect.setChunk(mutated_bank_bytes.data(),
                          static_cast<VstInt32>(mutated_bank_bytes.size()),
                          false) == 1,
          "Expected full bank chunk restore to succeed");

  effect.setProgram(4);
  std::memset(program_name, 0, sizeof(program_name));
  effect.getProgramName(program_name);
  require(std::string_view(program_name) == "Fixture Program",
          "Expected restored bank program name");
  require(almostEqual(effect.getParameter(2), 0.73F),
          "Expected restored bank program state");

  bank_chunk.chunk->programs.resize(1U);
  bank_chunk.chunk->programs[0].name = "Current Only";
  bank_chunk.chunk->programs[0].parameter_values[7] = 0.61F;
  auto mutated_preset_bytes = encodeLegacyChunk(bank_chunk.chunk.value());
  require(effect.setChunk(mutated_preset_bytes.data(),
                          static_cast<VstInt32>(mutated_preset_bytes.size()),
                          true) == 1,
          "Expected preset chunk restore to succeed");
  require(almostEqual(effect.getParameter(7), 0.61F),
          "Expected preset chunk to update the current program");
  std::memset(program_name, 0, sizeof(program_name));
  effect.getProgramName(program_name);
  require(std::string_view(program_name) == "Current Only",
          "Expected preset chunk to update current program name");

  effect.setSampleRate(48000.0F);
  effect.setBlockSize(8);
  effect.resume();
  effect.setParameter(0, 0.0F);
  effect.setParameter(3, 0.0F);
  effect.setParameter(6, 0.0F);
  effect.setParameter(9, 1.0F);
  effect.setParameter(10, 1.0F);
  effect.setParameter(11, 0.5F);

  float left_input[8]{};
  float right_input[8]{};
  right_input[0] = 0.65F;
  right_input[2] = -0.30F;
  float left_output[8]{};
  float right_output[8]{};
  float* inputs[2] = {left_input, right_input};
  float* outputs[2] = {left_output, right_output};

  effect.processReplacing(inputs, outputs, 8);
  require(almostEqual(left_output[0], 0.0F) &&
              almostEqual(left_output[2], 0.0F),
          "Expected silent left input to stay silent");
  require(almostEqual(right_output[0], 0.65F) &&
              almostEqual(right_output[2], -0.30F),
          "Expected right-only input to stay on the right channel");

  left_input[0] = 0.4F;
  left_input[1] = -0.2F;
  std::memset(right_input, 0, sizeof(right_input));
  std::memset(left_output, 0, sizeof(left_output));
  std::memset(right_output, 0, sizeof(right_output));
  require(effect.setSpeakerArrangement(&stereo_input, &stereo_output),
          "Expected stereo input with stereo output to remain accepted");
  effect.processReplacing(inputs, outputs, 8);
  require(almostEqual(left_output[0], 0.4F) &&
              almostEqual(left_output[1], -0.2F),
          "Expected left input to remain on the left output");
  require(almostEqual(right_output[0], 0.0F) &&
              almostEqual(right_output[1], 0.0F),
          "Expected silent right channel to remain silent in stereo");

  std::memset(left_output, 0, sizeof(left_output));
  std::memset(right_output, 0, sizeof(right_output));
  aeffect->process(aeffect, inputs, outputs, 8);
  require(almostEqual(left_output[0], 0.4F) &&
              almostEqual(left_output[1], -0.2F),
          "Expected legacy VST2 process path to keep left input on the left");
  require(almostEqual(right_output[0], 0.0F) &&
              almostEqual(right_output[1], 0.0F),
          "Expected legacy process path to preserve stereo silence");

  require(effect.setSpeakerArrangement(&mono_input, &stereo_output),
          "Expected mono input with stereo output to remain accepted");
  left_input[0] = 0.4F;
  left_input[1] = -0.2F;
  right_input[0] = 0.0F;
  right_input[1] = 0.0F;
  std::memset(left_output, 0, sizeof(left_output));
  std::memset(right_output, 0, sizeof(right_output));
  effect.processReplacing(inputs, outputs, 8);
  require(almostEqual(left_output[0], 0.4F) &&
              almostEqual(left_output[1], -0.2F),
          "Expected mono input to remain on the left output");
  require(almostEqual(right_output[0], 0.4F) &&
              almostEqual(right_output[1], -0.2F),
          "Expected mono-in stereo-out negotiation to mirror mono input to the right output");

  return 0;
}
