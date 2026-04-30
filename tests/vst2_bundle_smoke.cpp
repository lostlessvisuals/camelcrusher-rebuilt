#include "pluginterfaces/vst2.x/aeffect.h"

#include <dlfcn.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <string_view>

namespace {

using VstMainProc = AEffect* (*)(audioMasterCallback);

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

}  // namespace

int main() {
  const auto binary_path = std::filesystem::path(CAMELCRUSHER_BUILD_DIR) /
                           "CamelCrusher.vst" / "Contents" / "MacOS" /
                           "CamelCrusher";
  require(std::filesystem::exists(binary_path),
          "Expected packaged CamelCrusher.vst bundle binary");

  void* handle = dlopen(binary_path.c_str(), RTLD_NOW | RTLD_LOCAL);
  require(handle != nullptr, dlerror() != nullptr ? dlerror() : "Failed to dlopen VST2 bundle");

  const auto close_handle = [&]() { dlclose(handle); };

  auto* main_proc =
      reinterpret_cast<VstMainProc>(dlsym(handle, "VSTPluginMain"));
  require(main_proc != nullptr, "Expected VSTPluginMain export");

  AEffect* effect = main_proc(hostCallback);
  require(effect != nullptr, "Expected VSTPluginMain to create an effect");
  require(effect->uniqueID == 1130447730, "Expected legacy CaCr unique ID");
  require(effect->numPrograms == 20, "Expected 20 programs from packaged VST2");
  require(effect->numParams == 17, "Expected 17 params from packaged VST2");
  require(effect->dispatcher != nullptr, "Expected VST dispatcher");
  require(effect->processReplacing != nullptr,
          "Expected replacing process callback");

  char effect_name[kVstMaxEffectNameLen] = {};
  effect->dispatcher(effect, effGetEffectName, 0, 0, effect_name, 0.0F);
  require(std::string_view(effect_name) == "CamelCrusher",
          "Expected CamelCrusher bundle effect name");

  effect->dispatcher(effect, effClose, 0, 0, nullptr, 0.0F);
  close_handle();
  return 0;
}
