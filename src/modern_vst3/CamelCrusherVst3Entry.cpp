#include "CamelCrusherVst3Controller.h"
#include "CamelCrusherVst3IDs.h"
#include "CamelCrusherVst3Processor.h"
#include "CamelCrusherVst3Version.h"

#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

BEGIN_FACTORY_DEF(stringCompanyName, stringCompanyWeb, stringCompanyEmail)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(camelcrusher_recalled::vst3::kCamelCrusherVst3ProcessorUID),
    PClassInfo::kManyInstances,
    kVstAudioEffectClass,
    camelcrusher_recalled::vst3::kCamelCrusherPluginName,
    Vst::kDistributable,
    camelcrusher_recalled::vst3::kCamelCrusherPluginSubcategories,
    FULL_VERSION_STR,
    kVstVersionString,
    camelcrusher_recalled::vst3::CamelCrusherVst3Processor::createInstance)

DEF_CLASS2(
    INLINE_UID_FROM_FUID(camelcrusher_recalled::vst3::kCamelCrusherVst3ControllerUID),
    PClassInfo::kManyInstances,
    kVstComponentControllerClass,
    camelcrusher_recalled::vst3::kCamelCrusherControllerName,
    0,
    "",
    FULL_VERSION_STR,
    kVstVersionString,
    camelcrusher_recalled::vst3::CamelCrusherVst3Controller::createInstance)

END_FACTORY
