#pragma once

#include "camelcrusher_recalled/ModernPluginModel.h"

#include <array>
#include <cstddef>
#include <span>

namespace camelcrusher_recalled {

struct ModernProcessingControls {
  bool global_bypass = false;
  bool distortion_enabled = false;
  bool filter_enabled = false;
  bool compressor_enabled = false;
  bool master_section_enabled = false;
  bool compressor_phat_enabled = false;

  float distortion_mechanical_amount = 0.0F;
  float distortion_tube_amount = 0.0F;
  float distortion_pre_gain = 1.0F;
  float distortion_output_trim = 1.0F;
  float distortion_tube_bias = 0.0F;
  float filter_cutoff_hz = 20000.0F;
  float filter_resonance = 0.0F;
  float filter_drive = 1.0F;
  float compressor_threshold = 1.0F;
  float compressor_ratio = 1.0F;
  float compressor_attack_ms = 5.0F;
  float compressor_release_ms = 80.0F;
  float compressor_knee_db = 6.0F;
  float compressor_makeup_gain = 1.0F;
  float compressor_detector_blend = 0.5F;
  float compressor_saturation = 0.0F;
  float compressor_body_amount = 0.0F;
  float wet_mix = 1.0F;
  float output_gain = 1.0F;
  float output_limiter_ceiling = 0.98F;
  float output_limiter_release_ms = 90.0F;
};

struct ModernProcessSpec {
  double sample_rate = 44100.0;
  std::size_t max_block_size = 0;
  std::size_t channel_count = 2;
};

struct ModernAudioBlock {
  std::span<float> left;
  std::span<float> right;
};

ModernProcessingControls modernProcessingControlsFromState(
    const ModernPluginState& state);

class ModernProcessor {
 public:
  void prepare(const ModernProcessSpec& spec);
  void reset();
  void processBlock(ModernAudioBlock block, const ModernPluginState& state);

 private:
  float processDistortionSample(float input_sample,
                                std::size_t channel_index,
                                const ModernProcessingControls& controls);
  float processFilterSample(float input_sample,
                            std::size_t channel_index,
                            const ModernProcessingControls& controls);
  void processStereoCompressor(float& left_sample,
                               float& right_sample,
                               const ModernProcessingControls& controls);
  void processStereoOutputLimiter(float& left_sample,
                                  float& right_sample,
                                  const ModernProcessingControls& controls);
  static float clampAudioSample(float sample);

  double sample_rate_ = 44100.0;
  std::size_t channel_count_ = 2;
  std::array<float, 2> distortion_tone_state_{};
  std::array<float, 2> filter_ic1eq_{};
  std::array<float, 2> filter_ic2eq_{};
  std::array<float, 2> compressor_phat_state_{};
  float compressor_envelope_ = 0.0F;
  float compressor_gain_ = 1.0F;
  float output_limiter_gain_ = 1.0F;
};

}  // namespace camelcrusher_recalled
