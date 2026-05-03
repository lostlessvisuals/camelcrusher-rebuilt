#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

#include <cstdio>
#include <cstring>

namespace {

constexpr OSType fourCC(const char a,
                        const char b,
                        const char c,
                        const char d) {
  return (static_cast<OSType>(a) << 24U) | (static_cast<OSType>(b) << 16U) |
         (static_cast<OSType>(c) << 8U) | static_cast<OSType>(d);
}

bool flagPresent(int argc, char* argv[], const char* flag) {
  for (int index = 1; index < argc; ++index) {
    if (std::strcmp(argv[index], flag) == 0) {
      return true;
    }
  }
  return false;
}

}  // namespace

int main(int argc, char* argv[]) {
  const bool quiet = flagPresent(argc, argv, "--quiet");

  @autoreleasepool {
    AudioComponentDescription description{};
    description.componentType = kAudioUnitType_Effect;
    description.componentSubType = fourCC('C', 'c', 'r', 'R');
    description.componentManufacturer = fourCC('C', 'm', 'A', 'u');

    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    if (component == nullptr) {
      if (!quiet) {
        std::puts("CamelCrusher AU component not found.");
      }
      return 1;
    }

    NSArray<AVAudioUnitComponent*>* components =
        [AVAudioUnitComponentManager.sharedAudioUnitComponentManager
            componentsMatchingDescription:description];
    if (components.count == 0U) {
      if (!quiet) {
        std::puts("AudioComponent exists, but AVAudioUnitComponentManager returned no matches.");
      }
      return 2;
    }

    AVAudioUnitComponent* component_info = components.firstObject;
    if (quiet) {
      return 0;
    }

    std::printf("Found AU component: %s\n", component_info.name.UTF8String);
    std::printf("Type=%08x Subtype=%08x Manufacturer=%08x\n",
                static_cast<unsigned int>(description.componentType),
                static_cast<unsigned int>(description.componentSubType),
                static_cast<unsigned int>(description.componentManufacturer));
    std::printf("version=%lu\n",
                static_cast<unsigned long>(component_info.version));
    std::printf("hasCustomView=%d\n", component_info.hasCustomView ? 1 : 0);
    std::printf("passesAUVal=%d\n", component_info.passesAUVal ? 1 : 0);
    std::printf("sandboxSafe=%d\n", component_info.isSandboxSafe ? 1 : 0);
    std::printf("typeName=%s\n", component_info.typeName.UTF8String);

    NSDictionary<NSString*, id>* configuration =
        component_info.configurationDictionary;
    id custom_view_flag = configuration[@"HasCustomView"];
    if (custom_view_flag != nil) {
      std::printf("configuration.HasCustomView=%s\n",
                  [[custom_view_flag description] UTF8String]);
    }
  }

  return 0;
}
