#include "camelcrusher_recalled/ModernProcessor.h"

#include <algorithm>
#include <cmath>

namespace camelcrusher_recalled {
namespace {

constexpr float kPi = 3.14159265358979323846F;
constexpr float kEpsilon = 0.000001F;

float normalizedToSwitch(const float value) {
  return value >= 0.5F ? 1.0F : 0.0F;
}

float normalizedCutoffToHertz(const float normalized_value) {
  constexpr float minimum_hz = 40.0F;
  constexpr float maximum_hz = 18000.0F;
  const auto value = clampModernNormalizedValue(normalized_value);
  return minimum_hz *
         std::pow(maximum_hz / minimum_hz, value);
}

float timeToCoefficient(const float milliseconds, const double sample_rate) {
  const auto seconds = std::max(milliseconds, 0.1F) / 1000.0F;
  return std::exp(-1.0F / static_cast<float>(sample_rate * seconds));
}

float dbToLinear(const float decibels) {
  return std::pow(10.0F, decibels / 20.0F);
}

float linearToDb(const float linear) {
  return 20.0F * std::log10(std::max(linear, kEpsilon));
}

float softClip(const float sample) {
  return std::tanh(sample);
}

float waveFold(const float sample) {
  return std::asin(std::sin(sample)) * (2.0F / kPi);
}

float mix(const float dry, const float wet, const float amount) {
  return dry + (wet - dry) * amount;
}

float onePoleAlpha(const float cutoff_hz, const double sample_rate) {
  return 1.0F -
         std::exp((-2.0F * kPi * cutoff_hz) /
                  static_cast<float>(sample_rate));
}

float softKneeGainDb(const float level_db,
                     const float threshold_db,
                     const float ratio,
                     const float knee_db) {
  const auto knee_half = knee_db * 0.5F;
  if (level_db <= threshold_db - knee_half) {
    return 0.0F;
  }

  if (level_db >= threshold_db + knee_half) {
    return (threshold_db + (level_db - threshold_db) / ratio) - level_db;
  }

  const auto delta = level_db - (threshold_db - knee_half);
  const auto curve = (1.0F / ratio) - 1.0F;
  return curve * delta * delta / (2.0F * knee_db);
}

}  // namespace

ModernProcessingControls modernProcessingControlsFromState(
    const ModernPluginState& state) {
  ModernProcessingControls controls;

  controls.distortion_enabled = normalizedToSwitch(state.state.dist_on) > 0.5F;
  controls.filter_enabled = normalizedToSwitch(state.state.mm_filter_on) > 0.5F;
  controls.compressor_enabled =
      normalizedToSwitch(state.state.compress_on) > 0.5F;
  controls.master_section_enabled =
      normalizedToSwitch(state.state.master_on) > 0.5F;
  controls.global_bypass = !controls.master_section_enabled;

  controls.distortion_mechanical_amount =
      clampModernNormalizedValue(state.state.dist_mech);
  controls.distortion_tube_amount =
      clampModernNormalizedValue(state.state.dist_tube);
  controls.distortion_pre_gain =
      1.0F + controls.distortion_mechanical_amount * 8.5F +
      controls.distortion_tube_amount * 6.5F;
  controls.distortion_output_trim =
      1.0F /
      (1.0F + controls.distortion_mechanical_amount * 0.95F +
       controls.distortion_tube_amount * 0.65F);
  controls.distortion_tube_bias =
      (controls.distortion_tube_amount - 0.5F) * 0.22F;

  controls.filter_cutoff_hz =
      normalizedCutoffToHertz(state.state.mm_filter_cutoff);
  controls.filter_resonance =
      0.03F + clampModernNormalizedValue(state.state.mm_filter_res) * 0.95F;
  controls.filter_drive =
      1.0F + controls.filter_resonance * 0.65F +
      controls.distortion_mechanical_amount * 0.45F;

  const auto compressor_amount =
      clampModernNormalizedValue(state.state.compress_amount);
  const auto compressor_mode =
      clampModernNormalizedValue(state.state.compress_mode);
  controls.compressor_phat_enabled =
      normalizedToSwitch(compressor_mode) > 0.5F;

  const auto threshold_db =
      controls.compressor_phat_enabled ? (-30.0F + compressor_amount * 22.0F)
                                       : (-24.0F + compressor_amount * 18.0F);
  controls.compressor_threshold = dbToLinear(threshold_db);
  controls.compressor_ratio =
      controls.compressor_phat_enabled ? (4.0F + compressor_amount * 10.0F)
                                       : (2.0F + compressor_amount * 6.0F);
  controls.compressor_attack_ms =
      controls.compressor_phat_enabled ? (1.5F + (1.0F - compressor_amount) * 6.0F)
                                       : (8.0F + (1.0F - compressor_amount) * 24.0F);
  controls.compressor_release_ms =
      controls.compressor_phat_enabled ? (70.0F + (1.0F - compressor_amount) * 120.0F)
                                       : (120.0F + (1.0F - compressor_amount) * 220.0F);
  controls.compressor_knee_db =
      controls.compressor_phat_enabled ? (6.0F + compressor_amount * 6.0F)
                                       : (9.0F + compressor_amount * 5.0F);
  controls.compressor_makeup_gain =
      dbToLinear(controls.compressor_phat_enabled ? (compressor_amount * 11.0F)
                                                  : (compressor_amount * 7.0F));
  controls.compressor_detector_blend =
      controls.compressor_phat_enabled ? 0.30F : 0.88F;
  controls.compressor_saturation =
      controls.compressor_phat_enabled ? (0.18F + compressor_amount * 0.45F)
                                       : (0.02F + compressor_amount * 0.08F);
  controls.compressor_body_amount =
      controls.compressor_phat_enabled ? (0.10F + compressor_amount * 0.55F)
                                       : 0.0F;

  controls.wet_mix = controls.master_section_enabled
                         ? clampModernNormalizedValue(state.state.master_mix)
                         : 1.0F;
  controls.output_gain = controls.master_section_enabled
                             ? dbToLinear(
                                   (clampModernNormalizedValue(
                                        state.state.master_volume) -
                                    0.5F) *
                                   24.0F)
                             : 1.0F;
  controls.output_limiter_ceiling =
      controls.compressor_phat_enabled ? 0.965F : 0.98F;
  controls.output_limiter_release_ms =
      controls.compressor_phat_enabled ? 115.0F : 85.0F;

  return controls;
}

void ModernProcessor::prepare(const ModernProcessSpec& spec) {
  sample_rate_ = spec.sample_rate > 0.0 ? spec.sample_rate : 44100.0;
  channel_count_ = std::clamp<std::size_t>(spec.channel_count, 1U, 2U);
  reset();
}

void ModernProcessor::reset() {
  distortion_tone_state_.fill(0.0F);
  filter_ic1eq_.fill(0.0F);
  filter_ic2eq_.fill(0.0F);
  compressor_phat_state_.fill(0.0F);
  compressor_envelope_ = 0.0F;
  compressor_gain_ = 1.0F;
  output_limiter_gain_ = 1.0F;
}

void ModernProcessor::processBlock(const ModernAudioBlock block,
                                   const ModernPluginState& state) {
  const auto controls = modernProcessingControlsFromState(state);
  if (controls.global_bypass) {
    return;
  }

  const auto frame_count = block.right.empty()
                               ? block.left.size()
                               : std::min(block.left.size(), block.right.size());

  for (std::size_t frame = 0; frame < frame_count; ++frame) {
    const auto dry_left = block.left[frame];
    auto wet_left = dry_left;

    const auto has_stereo_right =
        !block.right.empty() && channel_count_ > 1U;
    const auto dry_right =
        has_stereo_right ? block.right[frame] : dry_left;
    auto wet_right = dry_right;

    if (controls.distortion_enabled) {
      wet_left = processDistortionSample(wet_left, 0, controls);
      wet_right = processDistortionSample(wet_right, 1, controls);
    }

    if (controls.filter_enabled) {
      wet_left = processFilterSample(wet_left, 0, controls);
      wet_right = processFilterSample(wet_right, 1, controls);
    }

    if (controls.compressor_enabled) {
      processStereoCompressor(wet_left, wet_right, controls);
    }

    auto output_left =
        mix(dry_left, wet_left, controls.wet_mix) * controls.output_gain;
    auto output_right =
        mix(dry_right, wet_right, controls.wet_mix) * controls.output_gain;
    processStereoOutputLimiter(output_left, output_right, controls);

    block.left[frame] = clampAudioSample(output_left);
    if (has_stereo_right) {
      block.right[frame] = clampAudioSample(output_right);
    }
  }
}

float ModernProcessor::processDistortionSample(
    const float input_sample,
    const std::size_t channel_index,
    const ModernProcessingControls& controls) {
  const auto preamped = input_sample * controls.distortion_pre_gain;

  const auto mechanical_drive =
      1.0F + controls.distortion_mechanical_amount * 8.0F;
  const auto mechanical_fold =
      waveFold(preamped * (1.0F + controls.distortion_mechanical_amount * 12.0F));
  const auto mechanical_rectified =
      std::copysign(mechanical_fold * mechanical_fold, mechanical_fold);
  const auto mechanical =
      mix(softClip(preamped * mechanical_drive),
          mix(mechanical_fold, mechanical_rectified,
              controls.distortion_mechanical_amount * 0.6F),
          0.35F + controls.distortion_mechanical_amount * 0.45F);

  const auto tube_drive = 1.0F + controls.distortion_tube_amount * 7.0F;
  const auto biased = preamped + controls.distortion_tube_bias;
  const auto bias_reference =
      std::tanh(controls.distortion_tube_bias * tube_drive);
  const auto tube =
      std::tanh(biased * tube_drive) - bias_reference +
      biased * biased * controls.distortion_tube_bias * 0.1F;

  const auto distortion_mix = std::clamp(
      0.15F + controls.distortion_mechanical_amount * 0.55F +
          controls.distortion_tube_amount * 0.55F,
      0.0F, 1.0F);
  const auto mechanical_weight =
      controls.distortion_mechanical_amount /
      std::max(controls.distortion_mechanical_amount +
                   controls.distortion_tube_amount,
               0.001F);
  auto shaped = mix(tube, mechanical, mechanical_weight);
  shaped = mix(input_sample, shaped, distortion_mix);

  const auto tone_cutoff =
      2400.0F + (1.0F - controls.distortion_tube_amount) * 7600.0F;
  const auto alpha = onePoleAlpha(tone_cutoff, sample_rate_);
  auto& tone_state = distortion_tone_state_[channel_index];
  tone_state += alpha * (shaped - tone_state);
  return tone_state * controls.distortion_output_trim;
}

float ModernProcessor::processFilterSample(
    const float input_sample,
    const std::size_t channel_index,
    const ModernProcessingControls& controls) {
  const auto cutoff =
      std::clamp(controls.filter_cutoff_hz, 40.0F, 20000.0F);
  const auto g =
      std::tan(kPi * cutoff / static_cast<float>(sample_rate_));
  const auto resonance = std::clamp(controls.filter_resonance, 0.01F, 0.99F);
  const auto k = 2.0F - 1.85F * resonance;
  const auto a1 = 1.0F / (1.0F + g * (g + k));
  const auto a2 = g * a1;
  const auto a3 = g * a2;

  auto& ic1eq = filter_ic1eq_[channel_index];
  auto& ic2eq = filter_ic2eq_[channel_index];

  const auto driven_input =
      std::tanh(input_sample * controls.filter_drive) / controls.filter_drive;
  const auto v3 = driven_input - ic2eq;
  const auto v1 = a1 * ic1eq + a2 * v3;
  const auto v2 = ic2eq + a2 * ic1eq + a3 * v3;
  ic1eq = std::tanh(2.0F * v1 - ic1eq);
  ic2eq = std::tanh(2.0F * v2 - ic2eq);

  const auto low = v2;
  const auto makeup = 1.0F + resonance * 0.35F;
  return low * makeup;
}

void ModernProcessor::processStereoCompressor(
    float& left_sample,
    float& right_sample,
    const ModernProcessingControls& controls) {
  const auto left_level = std::fabs(left_sample);
  const auto right_level = std::fabs(right_sample);
  const auto peak_level = std::max(left_level, right_level);
  const auto rms_level = std::sqrt((left_level * left_level +
                                    right_level * right_level) *
                                   0.5F);
  const auto detector_level =
      mix(peak_level, rms_level, controls.compressor_detector_blend);

  const auto attack_coeff =
      timeToCoefficient(controls.compressor_attack_ms, sample_rate_);
  const auto release_coeff =
      timeToCoefficient(controls.compressor_release_ms, sample_rate_);
  const auto envelope_coeff =
      detector_level > compressor_envelope_ ? attack_coeff : release_coeff;
  compressor_envelope_ =
      envelope_coeff * compressor_envelope_ +
      (1.0F - envelope_coeff) * detector_level;

  const auto level_db = linearToDb(compressor_envelope_);
  const auto threshold_db = linearToDb(controls.compressor_threshold);
  const auto gain_reduction_db =
      softKneeGainDb(level_db, threshold_db, controls.compressor_ratio,
                     controls.compressor_knee_db);
  const auto target_gain = dbToLinear(gain_reduction_db);

  const auto gain_coeff =
      target_gain < compressor_gain_ ? attack_coeff : release_coeff;
  compressor_gain_ = gain_coeff * compressor_gain_ +
                     (1.0F - gain_coeff) * target_gain;

  const auto applied_gain =
      compressor_gain_ * controls.compressor_makeup_gain;
  left_sample *= applied_gain;
  right_sample *= applied_gain;

  if (controls.compressor_phat_enabled) {
    const auto body_alpha =
        onePoleAlpha(180.0F + controls.compressor_body_amount * 140.0F,
                     sample_rate_);
    for (std::size_t channel = 0; channel < 2; ++channel) {
      auto& sample = channel == 0 ? left_sample : right_sample;
      auto& body_state = compressor_phat_state_[channel];
      body_state += body_alpha * (sample - body_state);
      const auto saturated = softClip(
          sample * (1.0F + controls.compressor_saturation * 5.0F) +
          body_state * controls.compressor_body_amount * 0.7F);
      sample = mix(sample, saturated,
                   0.25F + controls.compressor_saturation * 0.35F);
    }
  } else {
    const auto smoothing_mix = controls.compressor_saturation;
    left_sample = mix(left_sample, softClip(left_sample * 1.2F), smoothing_mix);
    right_sample =
        mix(right_sample, softClip(right_sample * 1.2F), smoothing_mix);
  }
}

void ModernProcessor::processStereoOutputLimiter(
    float& left_sample,
    float& right_sample,
    const ModernProcessingControls& controls) {
  const auto peak =
      std::max(std::fabs(left_sample), std::fabs(right_sample));
  const auto ceiling = std::max(controls.output_limiter_ceiling, 0.5F);
  const auto target_gain = peak > ceiling ? (ceiling / peak) : 1.0F;
  const auto release_coeff =
      timeToCoefficient(controls.output_limiter_release_ms, sample_rate_);

  if (target_gain < output_limiter_gain_) {
    output_limiter_gain_ = target_gain;
  } else {
    output_limiter_gain_ =
        release_coeff * output_limiter_gain_ +
        (1.0F - release_coeff) * target_gain;
  }

  left_sample *= output_limiter_gain_;
  right_sample *= output_limiter_gain_;
}

float ModernProcessor::clampAudioSample(const float sample) {
  return std::clamp(sample, -1.0F, 1.0F);
}

}  // namespace camelcrusher_recalled
