#include "camelcrusher_recalled/LegacyState.h"

#include <cmath>

namespace camelcrusher_recalled {
namespace {

bool nearlyEqual(const float left, const float right) {
  return std::fabs(left - right) < 0.000001F;
}

}  // namespace

LegacyState legacyStateFromArray(const std::span<const float> values) {
  LegacyState state;

  for (const auto& descriptor : legacyParameterDescriptors()) {
    if (descriptor.legacy_index < values.size()) {
      setLegacyStateValue(state, descriptor.id, values[descriptor.legacy_index]);
    }
  }

  return state;
}

LegacyParameterArray legacyStateToArray(const LegacyState& state) {
  LegacyParameterArray values{};

  for (const auto& descriptor : legacyParameterDescriptors()) {
    values[descriptor.legacy_index] = legacyStateValue(state, descriptor.id);
  }

  return values;
}

float legacyStateValue(const LegacyState& state, const LegacyParameterId id) {
  switch (id) {
    case LegacyParameterId::kDistOn:
      return state.dist_on;
    case LegacyParameterId::kDistMech:
      return state.dist_mech;
    case LegacyParameterId::kDistTube:
      return state.dist_tube;
    case LegacyParameterId::kMmFilterOn:
      return state.mm_filter_on;
    case LegacyParameterId::kMmFilterCutoff:
      return state.mm_filter_cutoff;
    case LegacyParameterId::kMmFilterRes:
      return state.mm_filter_res;
    case LegacyParameterId::kCompressOn:
      return state.compress_on;
    case LegacyParameterId::kCompressAmount:
      return state.compress_amount;
    case LegacyParameterId::kCompressMode:
      return state.compress_mode;
    case LegacyParameterId::kMasterOn:
      return state.master_on;
    case LegacyParameterId::kMasterMix:
      return state.master_mix;
    case LegacyParameterId::kMasterVolume:
      return state.master_volume;
    case LegacyParameterId::kUnused1:
      return state.unused_1;
    case LegacyParameterId::kUnused2:
      return state.unused_2;
    case LegacyParameterId::kUnused3:
      return state.unused_3;
    case LegacyParameterId::kUnused4:
      return state.unused_4;
    case LegacyParameterId::kUnused5:
      return state.unused_5;
  }

  return 0.0F;
}

void setLegacyStateValue(LegacyState& state,
                         const LegacyParameterId id,
                         const float value) {
  switch (id) {
    case LegacyParameterId::kDistOn:
      state.dist_on = value;
      return;
    case LegacyParameterId::kDistMech:
      state.dist_mech = value;
      return;
    case LegacyParameterId::kDistTube:
      state.dist_tube = value;
      return;
    case LegacyParameterId::kMmFilterOn:
      state.mm_filter_on = value;
      return;
    case LegacyParameterId::kMmFilterCutoff:
      state.mm_filter_cutoff = value;
      return;
    case LegacyParameterId::kMmFilterRes:
      state.mm_filter_res = value;
      return;
    case LegacyParameterId::kCompressOn:
      state.compress_on = value;
      return;
    case LegacyParameterId::kCompressAmount:
      state.compress_amount = value;
      return;
    case LegacyParameterId::kCompressMode:
      state.compress_mode = value;
      return;
    case LegacyParameterId::kMasterOn:
      state.master_on = value;
      return;
    case LegacyParameterId::kMasterMix:
      state.master_mix = value;
      return;
    case LegacyParameterId::kMasterVolume:
      state.master_volume = value;
      return;
    case LegacyParameterId::kUnused1:
      state.unused_1 = value;
      return;
    case LegacyParameterId::kUnused2:
      state.unused_2 = value;
      return;
    case LegacyParameterId::kUnused3:
      state.unused_3 = value;
      return;
    case LegacyParameterId::kUnused4:
      state.unused_4 = value;
      return;
    case LegacyParameterId::kUnused5:
      state.unused_5 = value;
      return;
  }

  return;
}

std::vector<LegacyParameterId> legacyStateMismatchIds(
    const LegacyState& state,
    const std::span<const float> values) {
  std::vector<LegacyParameterId> mismatches;

  for (const auto& descriptor : legacyParameterDescriptors()) {
    if (descriptor.legacy_index >= values.size() ||
        !nearlyEqual(legacyStateValue(state, descriptor.id),
                     values[descriptor.legacy_index])) {
      mismatches.push_back(descriptor.id);
    }
  }

  return mismatches;
}

bool legacyStateMatchesArray(const LegacyState& state,
                             const std::span<const float> values) {
  return legacyStateMismatchIds(state, values).empty();
}

}  // namespace camelcrusher_recalled
