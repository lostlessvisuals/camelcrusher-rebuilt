#pragma once

#include "camelcrusher_recalled/LegacyParameters.h"

#include <array>
#include <span>
#include <vector>

namespace camelcrusher_recalled {

using LegacyParameterArray = std::array<float, kLegacyParameterCount>;

struct LegacyState {
  float dist_on = 0.0F;
  float dist_mech = 0.0F;
  float dist_tube = 0.0F;
  float mm_filter_on = 0.0F;
  float mm_filter_cutoff = 0.0F;
  float mm_filter_res = 0.0F;
  float compress_on = 0.0F;
  float compress_amount = 0.0F;
  float compress_mode = 0.0F;
  float master_on = 0.0F;
  float master_mix = 0.0F;
  float master_volume = 0.0F;
  float unused_1 = 0.0F;
  float unused_2 = 0.0F;
  float unused_3 = 0.0F;
  float unused_4 = 0.0F;
  float unused_5 = 0.0F;
};

LegacyState legacyStateFromArray(std::span<const float> values);
LegacyParameterArray legacyStateToArray(const LegacyState& state);
float legacyStateValue(const LegacyState& state, LegacyParameterId id);
void setLegacyStateValue(LegacyState& state, LegacyParameterId id, float value);
std::vector<LegacyParameterId> legacyStateMismatchIds(
    const LegacyState& state,
    std::span<const float> values);
bool legacyStateMatchesArray(const LegacyState& state,
                             std::span<const float> values);

}  // namespace camelcrusher_recalled
