#pragma once

#include "TestSupport.h"
#include <algorithm>
#include <cmath>
#include <vector>

struct StereoCase {
  std::vector<float> left, right;
  StereoCase(int size, int pattern) : left(size, 0.25F), right(size, 0.0F) {
    if (pattern == 1) { std::fill(left.begin(), left.end(), 0.0F); right.back() = -0.5F; }
    if (pattern == 2) std::fill(right.begin(), right.end(), -0.125F);
    if (pattern == 3) right.back() = -0.5F;
    if (pattern == 4) std::fill(right.begin(), right.end(), 1.0e-8F);
    if (pattern == 5) std::fill(left.begin(), left.end(), 0.0F);
  }
};

inline void checkSample(float actual, float expected) {
  CC_CHECK(std::isfinite(actual));
  CC_CHECK(std::abs(actual - expected) < 1.0e-7F);
}
