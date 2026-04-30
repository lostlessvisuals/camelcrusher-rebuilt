#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace camelcrusher_recalled::vst3 {

inline constexpr char kCamelCrusherPluginName[] = "CamelCrusher";
inline constexpr char kCamelCrusherControllerName[] = "CamelCrusher Controller";
inline constexpr char kCamelCrusherPluginVendor[] = "Rivet";
inline constexpr char kCamelCrusherPluginSubcategories[] =
    "Fx|Distortion|Dynamics|Filter";

inline const Steinberg::FUID kCamelCrusherVst3ProcessorUID(
    0xE1DB144B, 0xBE744A7C, 0xAD98C738, 0x0690B8DB);
inline const Steinberg::FUID kCamelCrusherVst3ControllerUID(
    0xB436FC68, 0x5AA94EE2, 0x856B0E33, 0xE108C75B);

inline constexpr Steinberg::Vst::ParamID kCamelCrusherProgramParameterId = 1000;
inline constexpr Steinberg::int32 kCamelCrusherCurrentStateProgramSlot = 0;

}  // namespace camelcrusher_recalled::vst3
