#pragma once

#if defined(__APPLE__)

#import <AppKit/AppKit.h>

#include <array>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace camelcrusher_recalled {

inline constexpr std::size_t kCamelCrusherMacVisibleControlCount = 12;

enum class CamelCrusherMacVisibleControlRole : std::size_t {
  kDistOn = 0,
  kDistTube = 1,
  kDistMech = 2,
  kFilterOn = 3,
  kFilterCutoff = 4,
  kFilterRes = 5,
  kCompressOn = 6,
  kCompressAmount = 7,
  kCompressMode = 8,
  kMasterOn = 9,
  kMasterVolume = 10,
  kMasterMix = 11,
};

using CamelCrusherMacControlTagArray =
    std::array<NSInteger, kCamelCrusherMacVisibleControlCount>;
using CamelCrusherMacControlValueArray =
    std::array<float, kCamelCrusherMacVisibleControlCount>;

struct CamelCrusherMacEditorState {
  CamelCrusherMacControlValueArray control_values{};
  std::vector<std::string> program_names;
  NSInteger selected_program_index = 0;
  bool show_randomize_button = true;
};

class CamelCrusherMacEditorActionTarget {
 public:
  virtual ~CamelCrusherMacEditorActionTarget() = default;
  virtual void previousProgramRequested() = 0;
  virtual void nextProgramRequested() = 0;
  virtual void programSelectionChanged(NSInteger index) = 0;
  virtual void parameterControlChanged(NSInteger control_tag,
                                       float normalized_value) = 0;
  virtual void randomizeRequested() = 0;
};

class CamelCrusherMacEditorSurface {
 public:
  CamelCrusherMacEditorSurface(
      CamelCrusherMacEditorActionTarget* owner,
      const CamelCrusherMacControlTagArray& control_tags,
      NSString* background_resource_name = @"Background.png");
  ~CamelCrusherMacEditorSurface();

  CamelCrusherMacEditorSurface(const CamelCrusherMacEditorSurface&) = delete;
  CamelCrusherMacEditorSurface& operator=(const CamelCrusherMacEditorSurface&) =
      delete;

  NSView* view() const;
  void setFrame(NSRect frame);
  void setState(const CamelCrusherMacEditorState& state);

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace camelcrusher_recalled

#endif  // defined(__APPLE__)
