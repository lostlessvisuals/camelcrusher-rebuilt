#import <AppKit/AppKit.h>
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/AUViewController.h>

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

struct Arguments {
  bool quiet = false;
  NSString* screenshotPath = @"/tmp/camelcrusher-au-view-probe.png";
};

Arguments parseArguments(int argc, char* argv[]) {
  Arguments arguments;
  for (int index = 1; index < argc; ++index) {
    if (std::strcmp(argv[index], "--quiet") == 0) {
      arguments.quiet = true;
      continue;
    }
    if (std::strcmp(argv[index], "--screenshot") == 0 && index + 1 < argc) {
      arguments.screenshotPath =
          [NSString stringWithUTF8String:argv[++index]];
    }
  }
  return arguments;
}

}  // namespace

@interface CamelCrusherAUViewProbeAppDelegate : NSObject <NSApplicationDelegate> {
 @private
  Arguments _arguments;
  int _exitCode;
}
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, assign) Arguments arguments;
@property(nonatomic, assign) int exitCode;
@end

@implementation CamelCrusherAUViewProbeAppDelegate

@synthesize arguments = _arguments;
@synthesize exitCode = _exitCode;

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
  (void)notification;

  AudioComponentDescription description{};
  description.componentType = kAudioUnitType_Effect;
  description.componentSubType = fourCC('C', 'c', 'r', 'R');
  description.componentManufacturer = fourCC('R', 'v', 'F', 'x');

  if (!self.arguments.quiet) {
    std::puts("Instantiating CamelCrusher AU...");
  }

  [AUAudioUnit instantiateWithComponentDescription:description
                                           options:0
                                 completionHandler:^(
                                     AUAudioUnit* _Nullable unit,
                                     NSError* _Nullable error) {
                                   if (unit == nil) {
                                     if (!self.arguments.quiet) {
                                       NSString* message =
                                           error.localizedDescription ?: @"Unknown error";
                                       std::fprintf(stderr,
                                                    "Failed to instantiate AU: %s\n",
                                                    message.UTF8String);
                                     }
                                     self.exitCode = 1;
                                     [NSApp terminate:nil];
                                     return;
                                   }

                                   if (!self.arguments.quiet) {
                                     std::printf("providesUserInterface=%d\n",
                                                 unit.providesUserInterface ? 1 : 0);
                                   }

                                   [unit requestViewControllerWithCompletionHandler:^(
                                             AUViewControllerBase* _Nullable viewController) {
                                     if (viewController == nil) {
                                       if (!self.arguments.quiet) {
                                         std::fputs("Failed to retrieve AU custom view controller.\n",
                                                    stderr);
                                       }
                                       self.exitCode = 2;
                                       [NSApp terminate:nil];
                                       return;
                                     }

                                     if (!self.arguments.quiet) {
                                       std::printf("viewControllerClass=%s\n",
                                                   NSStringFromClass(viewController.class)
                                                       .UTF8String);
                                     }

                                     NSSize preferred_size = viewController.preferredContentSize;
                                     if (preferred_size.width <= 0.0 ||
                                         preferred_size.height <= 0.0) {
                                       preferred_size = NSMakeSize(345.0, 373.0);
                                     }

                                     self.window = [[NSWindow alloc]
                                         initWithContentRect:NSMakeRect(
                                                             160.0, 160.0,
                                                             preferred_size.width,
                                                             preferred_size.height)
                                                   styleMask:NSWindowStyleMaskTitled |
                                                             NSWindowStyleMaskClosable |
                                                             NSWindowStyleMaskMiniaturizable
                                                     backing:NSBackingStoreBuffered
                                                       defer:NO];
                                     self.window.title = @"CamelCrusher AU View Probe";
                                     self.window.contentViewController = viewController;
                                     [self.window center];
                                     [self.window makeKeyAndOrderFront:nil];
                                     [NSApp activateIgnoringOtherApps:YES];

                                     dispatch_after(
                                         dispatch_time(DISPATCH_TIME_NOW,
                                                       static_cast<int64_t>(2.0 * NSEC_PER_SEC)),
                                         dispatch_get_main_queue(), ^{
                                           NSView* content_view = self.window.contentView;
                                           if (content_view != nil &&
                                               self.arguments.screenshotPath.length > 0U) {
                                             NSBitmapImageRep* bitmap =
                                                 [content_view bitmapImageRepForCachingDisplayInRect:
                                                                   content_view.bounds];
                                             [content_view cacheDisplayInRect:content_view.bounds
                                                                          toBitmapImageRep:bitmap];
                                             NSData* png =
                                                 [bitmap representationUsingType:NSBitmapImageFileTypePNG
                                                                       properties:@{}];
                                             [png writeToFile:self.arguments.screenshotPath
                                                   atomically:YES];
                                             if (!self.arguments.quiet) {
                                               std::printf("wrote %s\n",
                                                           self.arguments.screenshotPath.UTF8String);
                                             }
                                           }

                                           self.exitCode = 0;
                                           [NSApp terminate:nil];
                                         });
                                   }];
                                 }];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
  (void)sender;
  return YES;
}

@end

int main(int argc, char* argv[]) {
  @autoreleasepool {
    NSApplication* application = [NSApplication sharedApplication];
    CamelCrusherAUViewProbeAppDelegate* delegate =
        [CamelCrusherAUViewProbeAppDelegate new];
    delegate.arguments = parseArguments(argc, argv);
    delegate.exitCode = 0;
    application.delegate = delegate;
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
    [application run];
    return delegate.exitCode;
  }
}
