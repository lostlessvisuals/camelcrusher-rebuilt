#include "CamelCrusherVst3IDs.h"

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/module.h"

#include "TestSupport.h"
#include "Vst3StereoContract.h"
#include <string>

int main(int argc, char** argv) {
  const char* bundle_path = argc == 2 ? argv[1] : CAMELCRUSHER_VST3_BUNDLE_PATH;
  std::string error;
  CC_CHECK(VST3::Hosting::Module::validateBundleStructure(
      bundle_path, error));
  auto module =
      VST3::Hosting::Module::create(bundle_path, error);
  CC_CHECK(module != nullptr);

  const auto factory = module->getFactory();
  const auto info = factory.info();
  CC_CHECK(info.vendor() == camelcrusher_recalled::vst3::kCamelCrusherPluginVendor);

  const auto classes = factory.classInfos();
  CC_CHECK(classes.size() == 2U);

  bool saw_processor = false;
  bool saw_controller = false;
  for (const auto& class_info : classes) {
    if (class_info.category() == kVstAudioEffectClass) {
      saw_processor = true;
      CC_CHECK(class_info.name() ==
             camelcrusher_recalled::vst3::kCamelCrusherPluginName);
      CC_CHECK(class_info.subCategoriesString() ==
             camelcrusher_recalled::vst3::kCamelCrusherPluginSubcategories);
      auto component =
          factory.createInstance<Steinberg::Vst::IComponent>(class_info.ID());
      CC_CHECK(component != nullptr);
      checkVst3Stereo(component.get());
    } else if (class_info.category() == kVstComponentControllerClass) {
      saw_controller = true;
      CC_CHECK(class_info.name() ==
             camelcrusher_recalled::vst3::kCamelCrusherControllerName);
      auto controller =
          factory.createInstance<Steinberg::Vst::IEditController>(class_info.ID());
      CC_CHECK(controller != nullptr);
      auto view = controller->createView(Steinberg::Vst::ViewType::kEditor);
      CC_CHECK(view != nullptr);
      CC_CHECK(view->isPlatformTypeSupported(Steinberg::kPlatformTypeNSView) ==
             Steinberg::kResultTrue);
      view->release();
    }
  }

  CC_CHECK(saw_processor);
  CC_CHECK(saw_controller);
  const auto module_info_path =
      VST3::Hosting::Module::getModuleInfoPath(bundle_path);
  CC_CHECK(static_cast<bool>(module_info_path));
  return 0;
}
