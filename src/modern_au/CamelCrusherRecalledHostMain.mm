#import <AppKit/AppKit.h>

@interface CamelCrusherRecalledHostAppDelegate : NSObject <NSApplicationDelegate>
@end

@implementation CamelCrusherRecalledHostAppDelegate
@end

int main(int argc, const char* argv[]) {
  @autoreleasepool {
    NSApplication* app = [NSApplication sharedApplication];
    [app setActivationPolicy:NSApplicationActivationPolicyProhibited];
    CamelCrusherRecalledHostAppDelegate* delegate =
        [[CamelCrusherRecalledHostAppDelegate alloc] init];
    [app setDelegate:delegate];
    [app run];
  }
  return 0;
}
