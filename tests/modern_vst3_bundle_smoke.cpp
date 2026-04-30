#include "CamelCrusherVst3IDs.h"

#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "public.sdk/source/vst/hosting/module.h"

#include <cassert>
#include <string>

int main() {
  std::string error;
  assert(VST3::Hosting::Module::validateBundleStructure(
      CAMELCRUSHER_VST3_BUNDLE_PATH, error));
  auto module =
      VST3::Hosting::Module::create(CAMELCRUSHER_VST3_BUNDLE_PATH, error);
  assert(module != nullptr);

  const auto factory = module->getFactory();
  const auto info = factory.info();
  assert(info.vendor() == camelcrusher_recalled::vst3::kCamelCrusherPluginVendor);

  const auto classes = factory.classInfos();
  assert(classes.size() == 2U);

  bool saw_processor = false;
  bool saw_controller = false;
  for (const auto& class_info : classes) {
    if (class_info.category() == kVstAudioEffectClass) {
      saw_processor = true;
      assert(class_info.name() ==
             camelcrusher_recalled::vst3::kCamelCrusherPluginName);
      assert(class_info.subCategoriesString() ==
             camelcrusher_recalled::vst3::kCamelCrusherPluginSubcategories);
      auto component =
          factory.createInstance<Steinberg::Vst::IComponent>(class_info.ID());
      assert(component != nullptr);
    } else if (class_info.category() == kVstComponentControllerClass) {
      saw_controller = true;
      assert(class_info.name() ==
             camelcrusher_recalled::vst3::kCamelCrusherControllerName);
      auto controller =
          factory.createInstance<Steinberg::Vst::IEditController>(class_info.ID());
      assert(controller != nullptr);
      auto view = controller->createView(Steinberg::Vst::ViewType::kEditor);
      assert(view != nullptr);
      assert(view->isPlatformTypeSupported(Steinberg::kPlatformTypeNSView) ==
             Steinberg::kResultTrue);
      view->release();
    }
  }

  assert(saw_processor);
  assert(saw_controller);
  const auto module_info_path =
      VST3::Hosting::Module::getModuleInfoPath(CAMELCRUSHER_VST3_BUNDLE_PATH);
  assert(static_cast<bool>(module_info_path));
  return 0;
}
