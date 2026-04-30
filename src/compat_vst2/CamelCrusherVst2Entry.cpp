#include "CamelCrusherVst2Effect.h"

#include "audioeffect.h"

using camelcrusher_recalled::createCamelCrusherVst2Effect;

#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#define CAMELCRUSHER_VST2_EXPORT __attribute__((visibility("default")))
#else
#define CAMELCRUSHER_VST2_EXPORT
#endif

AudioEffect* createEffectInstance(audioMasterCallback audioMaster) {
  return createCamelCrusherVst2Effect(audioMaster);
}

extern "C" {

CAMELCRUSHER_VST2_EXPORT AEffect* VSTPluginMain(audioMasterCallback audioMaster) {
  if (audioMaster == nullptr ||
      !audioMaster(nullptr, audioMasterVersion, 0, 0, nullptr, 0.0F)) {
    return nullptr;
  }

  AudioEffect* effect = createEffectInstance(audioMaster);
  return effect != nullptr ? effect->getAeffect() : nullptr;
}

CAMELCRUSHER_VST2_EXPORT AEffect* main_macho(audioMasterCallback audioMaster) {
  return VSTPluginMain(audioMaster);
}

}  // extern "C"
