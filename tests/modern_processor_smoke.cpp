#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/ModernPluginModel.h"
#include "camelcrusher_recalled/ModernProcessor.h"
#include "camelcrusher_recalled/ModernRuntime.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

using camelcrusher_recalled::decodeLegacyChunk;
using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::makeDefaultModernPluginState;
using camelcrusher_recalled::modernPluginStateFromLegacyImport;
using camelcrusher_recalled::modernProcessingControlsFromState;
using camelcrusher_recalled::ModernAudioBlock;
using camelcrusher_recalled::ModernPluginState;
using camelcrusher_recalled::ModernProcessSpec;
using camelcrusher_recalled::ModernProcessor;
using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyProgram;

LegacyProgram makeProgram(const std::string& name, const float seed) {
  LegacyProgram program;
  program.record_marker = 1.0F;
  program.name = name;
  for (std::size_t index = 0; index < program.parameter_values.size(); ++index) {
    program.parameter_values[index] =
        seed + static_cast<float>(index) * 0.03125F;
  }
  return program;
}

bool nearlyEqual(const float left, const float right, const float epsilon = 0.000001F) {
  return std::fabs(left - right) < epsilon;
}

float peakMagnitude(const std::vector<float>& samples) {
  float peak = 0.0F;
  for (const auto sample : samples) {
    peak = std::max(peak, std::fabs(sample));
  }
  return peak;
}

float sumAbsoluteDifference(const std::vector<float>& left,
                            const std::vector<float>& right) {
  const auto count = std::min(left.size(), right.size());
  float total = 0.0F;
  for (std::size_t index = 0; index < count; ++index) {
    total += std::fabs(left[index] - right[index]);
  }
  return total;
}

}  // namespace

int main() {
  LegacyChunk synthetic_chunk;
  synthetic_chunk.programs.push_back(makeProgram("Neutral", 0.10F));
  synthetic_chunk.programs.push_back(makeProgram("Driven", 0.20F));

  const auto encoded = encodeLegacyChunk(synthetic_chunk);
  const auto decoded = decodeLegacyChunk(encoded);
  assert(decoded.ok());
  assert(decoded.chunk.has_value());

  auto explicit_values = decoded.chunk->programs[1].parameter_values;
  explicit_values[0] = 1.0F;   // dist on
  explicit_values[1] = 0.85F;  // dist mech
  explicit_values[2] = 0.75F;  // dist tube
  explicit_values[3] = 1.0F;   // filter on
  explicit_values[4] = 0.15F;  // low cutoff
  explicit_values[5] = 0.60F;  // resonance
  explicit_values[6] = 1.0F;   // compressor on
  explicit_values[7] = 0.70F;  // amount
  explicit_values[8] = 0.40F;  // mode
  explicit_values[9] = 1.0F;   // master on
  explicit_values[10] = 0.65F; // mix
  explicit_values[11] = 0.80F; // volume

  const auto imported = importLegacyState(decoded.chunk.value(), 1, explicit_values);
  ModernPluginState modern_state = modernPluginStateFromLegacyImport(imported);
  const auto controls = modernProcessingControlsFromState(modern_state);

  assert(controls.distortion_enabled);
  assert(controls.filter_enabled);
  assert(controls.compressor_enabled);
  assert(controls.master_section_enabled);
  assert(!controls.global_bypass);
  assert(!controls.compressor_phat_enabled);
  assert(controls.distortion_pre_gain > 1.0F);
  assert(controls.distortion_mechanical_amount > 0.0F);
  assert(controls.distortion_tube_amount > 0.0F);
  assert(controls.filter_cutoff_hz < 1000.0F);
  assert(controls.filter_drive > 1.0F);
  assert(controls.wet_mix > 0.0F && controls.wet_mix < 1.0F);
  assert(controls.output_gain > 1.0F);

  ModernProcessor processor;
  processor.prepare(ModernProcessSpec{
      .sample_rate = 48000.0,
      .max_block_size = 16,
      .channel_count = 2,
  });

  std::vector<float> left(16, 0.0F);
  std::vector<float> right(16, 0.0F);
  left[0] = 0.8F;
  right[0] = 0.8F;

  processor.processBlock(ModernAudioBlock{
                             .left = left,
                             .right = right,
                         },
                         modern_state);

  assert(!nearlyEqual(left[0], 0.8F));
  assert(!nearlyEqual(right[0], 0.8F));
  assert(nearlyEqual(left[0], right[0], 0.00001F));
  assert(peakMagnitude(left) <= 1.0F);
  assert(peakMagnitude(right) <= 1.0F);

  processor.reset();
  std::vector<float> linked_left(24, 0.35F);
  std::vector<float> linked_right(24, 0.35F);
  linked_left[0] = 0.95F;

  processor.processBlock(ModernAudioBlock{
                             .left = linked_left,
                             .right = linked_right,
                         },
                         modern_state);

  assert(linked_left[0] > linked_right[0]);
  assert(linked_right[1] < 0.35F);

  processor.reset();
  ModernPluginState right_only_state = makeDefaultModernPluginState();
  right_only_state.state.master_on = 1.0F;
  right_only_state.state.master_mix = 1.0F;
  right_only_state.state.master_volume = 0.5F;
  right_only_state.state.dist_on = 1.0F;
  right_only_state.state.dist_mech = 0.80F;
  right_only_state.state.dist_tube = 0.60F;
  right_only_state.state.mm_filter_on = 1.0F;
  right_only_state.state.mm_filter_cutoff = 0.35F;
  right_only_state.state.mm_filter_res = 0.40F;
  right_only_state.state.compress_on = 1.0F;
  right_only_state.state.compress_amount = 0.70F;
  right_only_state.state.compress_mode = 0.0F;

  std::vector<float> right_only_left(16, 0.0F);
  std::vector<float> right_only_right(16, 0.0F);
  right_only_right[0] = 0.65F;
  right_only_right[2] = -0.30F;

  processor.processBlock(ModernAudioBlock{
                             .left = right_only_left,
                             .right = right_only_right,
                         },
                         right_only_state);

  assert(peakMagnitude(right_only_left) < 0.000001F);
  assert(!nearlyEqual(right_only_right[0], 0.0F));
  assert(!nearlyEqual(right_only_right[2], 0.0F));

  processor.reset();
  ModernPluginState bypass_state = modern_state;
  bypass_state.state.master_on = 0.0F;
  bypass_state.state.master_mix = 1.0F;
  bypass_state.state.master_volume = 0.5F;

  std::vector<float> bypass_left{0.10F, -0.20F, 0.30F, -0.40F};
  std::vector<float> bypass_right{0.10F, -0.20F, 0.30F, -0.40F};
  const auto dry_reference = bypass_left;

  processor.processBlock(ModernAudioBlock{
                             .left = bypass_left,
                             .right = bypass_right,
                         },
                         bypass_state);

  for (std::size_t index = 0; index < dry_reference.size(); ++index) {
    assert(nearlyEqual(bypass_left[index], dry_reference[index], 0.00001F));
    assert(nearlyEqual(bypass_right[index], dry_reference[index], 0.00001F));
  }

  processor.reset();
  ModernPluginState smooth_compressor_state = modern_state;
  smooth_compressor_state.state.dist_on = 0.0F;
  smooth_compressor_state.state.mm_filter_on = 0.0F;
  smooth_compressor_state.state.compress_on = 1.0F;
  smooth_compressor_state.state.compress_amount = 0.82F;
  smooth_compressor_state.state.compress_mode = 0.0F;
  smooth_compressor_state.state.master_on = 1.0F;
  smooth_compressor_state.state.master_mix = 1.0F;
  smooth_compressor_state.state.master_volume = 0.72F;

  ModernPluginState phat_compressor_state = smooth_compressor_state;
  phat_compressor_state.state.compress_mode = 1.0F;

  std::vector<float> smooth_left(48, 0.0F);
  std::vector<float> smooth_right(48, 0.0F);
  std::vector<float> phat_left(48, 0.0F);
  std::vector<float> phat_right(48, 0.0F);
  for (std::size_t index = 0; index < smooth_left.size(); ++index) {
    const auto phase = static_cast<float>(index) / static_cast<float>(smooth_left.size());
    const auto sample = std::sin(phase * 6.2831853F) * 0.78F;
    smooth_left[index] = sample;
    smooth_right[index] = sample * 0.92F;
    phat_left[index] = smooth_left[index];
    phat_right[index] = smooth_right[index];
  }

  processor.processBlock(ModernAudioBlock{
                             .left = smooth_left,
                             .right = smooth_right,
                         },
                         smooth_compressor_state);
  processor.reset();
  processor.processBlock(ModernAudioBlock{
                             .left = phat_left,
                             .right = phat_right,
                         },
                         phat_compressor_state);

  assert(sumAbsoluteDifference(smooth_left, phat_left) > 0.25F);
  assert(sumAbsoluteDifference(smooth_right, phat_right) > 0.25F);
  assert(peakMagnitude(phat_left) <= 1.0F);
  assert(peakMagnitude(phat_right) <= 1.0F);

  std::cout << "Modern processor smoke test passed\n";
  return 0;
}
