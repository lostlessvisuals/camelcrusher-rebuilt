#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/gui/iplugview.h"

namespace camelcrusher_recalled::vst3 {

class CamelCrusherVst3Controller;

class CamelCrusherVst3EditorObserver {
 public:
  virtual ~CamelCrusherVst3EditorObserver() = default;
  virtual void vst3ControllerDidChangeState() = 0;
};

Steinberg::IPlugView* createCamelCrusherVst3Editor(
    CamelCrusherVst3Controller& controller);

}  // namespace camelcrusher_recalled::vst3
