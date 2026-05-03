#import <AudioToolbox/AUAudioUnitImplementation.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/AUViewController.h>
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

bool waitForSemaphoreWithRunLoop(dispatch_semaphore_t semaphore,
                                 const NSTimeInterval timeout_seconds) {
  NSDate* deadline =
      [NSDate dateWithTimeIntervalSinceNow:timeout_seconds];
  while ([deadline timeIntervalSinceNow] > 0.0) {
    if (dispatch_semaphore_wait(semaphore, DISPATCH_TIME_NOW) == 0) {
      return true;
    }

    NSDate* slice_deadline =
        [NSDate dateWithTimeIntervalSinceNow:0.05];
    [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode
                             beforeDate:slice_deadline];
  }

  return dispatch_semaphore_wait(semaphore, DISPATCH_TIME_NOW) == 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  const bool quiet = flagPresent(argc, argv, "--quiet");
  const bool load_in_process = flagPresent(argc, argv, "--in-process");
  const bool dump_state = flagPresent(argc, argv, "--dump-state");

  @autoreleasepool {
    AudioComponentDescription description{};
    description.componentType = kAudioUnitType_Effect;
    description.componentSubType = fourCC('C', 'c', 'r', 'R');
    description.componentManufacturer = fourCC('C', 'm', 'A', 'u');

    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    __block AUAudioUnit* instantiated_unit = nil;
    __block NSError* instantiate_error = nil;

    AudioComponentInstantiationOptions options = 0;
    if (load_in_process) {
      options |= kAudioComponentInstantiation_LoadInProcess;
    }

    [AUAudioUnit instantiateWithComponentDescription:description
                                             options:options
                                   completionHandler:^(
                                       AUAudioUnit* _Nullable unit,
                                       NSError* _Nullable error) {
                                     instantiated_unit = unit;
                                     instantiate_error = error;
                                     dispatch_semaphore_signal(semaphore);
                                   }];

    if (!waitForSemaphoreWithRunLoop(semaphore, 5.0)) {
      if (!quiet) {
        std::fputs("Timed out waiting for AUAudioUnit instantiation.\n", stderr);
      }
      return 3;
    }

    if (instantiated_unit == nil) {
      if (!quiet) {
        NSString* description_text =
            instantiate_error.localizedDescription ?: @"Unknown error";
        std::fprintf(stderr, "Failed to instantiate AU component: %s\n",
                     description_text.UTF8String);
      }
      return 1;
    }

    if (!quiet) {
      std::printf("Instantiated AU component with %lu public parameters.\n",
                  static_cast<unsigned long>(
                      instantiated_unit.parameterTree.allParameters.count));
      std::printf("providesUserInterface=%d\n",
                  instantiated_unit.providesUserInterface ? 1 : 0);
      if (dump_state) {
        NSDictionary<NSString*, id>* full_state = instantiated_unit.fullState;
        std::printf("fullState keys: %s\n",
                    [[full_state.allKeys componentsJoinedByString:@","]
                        UTF8String]);
      }
    }

    dispatch_semaphore_t view_semaphore = dispatch_semaphore_create(0);
    __block AUViewControllerBase* requested_view_controller = nil;
    [instantiated_unit requestViewControllerWithCompletionHandler:^(
                           AUViewControllerBase* _Nullable viewController) {
      requested_view_controller = viewController;
      dispatch_semaphore_signal(view_semaphore);
    }];

    if (!waitForSemaphoreWithRunLoop(view_semaphore, 5.0) ||
        requested_view_controller == nil) {
      if (!quiet) {
        std::fputs("Failed to retrieve AU custom view controller.\n", stderr);
      }
      return 4;
    }

    if (!quiet) {
      std::printf("viewControllerClass=%s\n",
                  NSStringFromClass(requested_view_controller.class).UTF8String);
    }
  }

  return 0;
}
