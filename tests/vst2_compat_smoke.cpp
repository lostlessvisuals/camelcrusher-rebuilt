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
  require(aeffect->flags == 57, "Expected original CamelCrusher raw VST2 flags");
  require((aeffect->flags & effFlagsProgramChunks) != 0,
          "Expected chunk-based VST2 state");
  require(effect.getPlugCategory() == kPlugCategUnknown,
          "Expected original CamelCrusher plug-in category");

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

  return 0;
}
