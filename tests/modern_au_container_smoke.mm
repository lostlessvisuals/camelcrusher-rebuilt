#import <Foundation/Foundation.h>

#include "TestSupport.h"
#include <fstream>

#include <mach-o/loader.h>

namespace {

bool hasExecutableMachOType(NSString* executable_path, const uint32_t filetype) {
  std::ifstream stream([executable_path fileSystemRepresentation],
                       std::ios::binary);
  mach_header_64 header{};
  stream.read(reinterpret_cast<char*>(&header), sizeof(header));
  return stream.good() && header.magic == MH_MAGIC_64 &&
         header.filetype == filetype;
}

}  // namespace

int main() {
  @autoreleasepool {
    NSString* app_path = @CAMELCRUSHER_AU_HOST_APP_PATH;
    NSString* embedded_bundle_path = @CAMELCRUSHER_AU_EMBEDDED_BUNDLE_PATH;
    NSFileManager* file_manager = [NSFileManager defaultManager];

    CC_CHECK([file_manager fileExistsAtPath:app_path]);
    CC_CHECK([file_manager fileExistsAtPath:embedded_bundle_path]);

    NSBundle* app_bundle = [NSBundle bundleWithPath:app_path];
    CC_CHECK(app_bundle != nil);
    NSDictionary* app_info = app_bundle.infoDictionary;
    CC_CHECK(app_info != nil);
    CC_CHECK([app_info[@"CFBundlePackageType"] isEqualToString:@"APPL"]);
    CC_CHECK([app_info[@"CFBundleDisplayName"]
        isEqualToString:@"CamelCrusher Host"]);
    CC_CHECK([app_info[@"CFBundleIdentifier"]
        isEqualToString:@"dev.rivet.camelcrusher-recalled.host"]);

    NSBundle* extension_bundle = [NSBundle bundleWithPath:embedded_bundle_path];
    CC_CHECK(extension_bundle != nil);
    NSDictionary* extension_info = extension_bundle.infoDictionary;
    CC_CHECK(extension_info != nil);
    CC_CHECK([extension_info[@"CFBundleDisplayName"]
        isEqualToString:@"CamelCrusher"]);

    NSDictionary* extension = extension_info[@"NSExtension"];
    CC_CHECK(extension != nil);
    CC_CHECK([extension[@"NSExtensionPointIdentifier"]
        isEqualToString:@"com.apple.AudioUnit-UI"]);
    CC_CHECK([extension[@"NSExtensionPrincipalClass"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);

    NSDictionary* attributes = extension[@"NSExtensionAttributes"];
    CC_CHECK(attributes != nil);
    NSArray* components = attributes[@"AudioComponents"];
    CC_CHECK(components.count == 1U);
    NSDictionary* component = components.firstObject;
    CC_CHECK([component[@"description"] isEqualToString:@"CamelCrusher"]);
    CC_CHECK([component[@"factoryFunction"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);
    CC_CHECK([component[@"name"] isEqualToString:@"Camel Audio: CamelCrusher"]);
    CC_CHECK([component[@"version"] isEqual:@(CAMELCRUSHER_EXPECTED_BUILD)]);

    NSString* executable_path = [embedded_bundle_path
        stringByAppendingPathComponent:@"Contents/MacOS/CamelCrusherAU"];
    CC_CHECK([file_manager fileExistsAtPath:executable_path]);
    CC_CHECK(hasExecutableMachOType(executable_path, MH_EXECUTE));
  }

  return 0;
}
