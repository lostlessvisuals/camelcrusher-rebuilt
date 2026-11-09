#pragma once

#include <cstdlib>
#include <iostream>

// Unlike assert(), checks and their expressions run in every build configuration.
#define CC_CHECK(...)                                                        \
  do {                                                                      \
    if (!(__VA_ARGS__)) {                                                    \
      std::cerr << __FILE__ << ':' << __LINE__ << ": " << #__VA_ARGS__ << '\n'; \
      std::abort();                                                         \
    }                                                                       \
  } while (false)
