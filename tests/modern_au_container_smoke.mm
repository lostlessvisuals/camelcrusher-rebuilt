#import <Foundation/Foundation.h>

#include <cassert>
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

    assert([file_manager fileExistsAtPath:app_path]);
    assert([file_manager fileExistsAtPath:embedded_bundle_path]);

    NSBundle* app_bundle = [NSBundle bundleWithPath:app_path];
    assert(app_bundle != nil);
    NSDictionary* app_info = app_bundle.infoDictionary;
    assert(app_info != nil);
    assert([app_info[@"CFBundlePackageType"] isEqualToString:@"APPL"]);
    assert([app_info[@"CFBundleDisplayName"]
        isEqualToString:@"CamelCrusher Host"]);
    assert([app_info[@"CFBundleIdentifier"]
        isEqualToString:@"dev.rivet.camelcrusher-recalled.host"]);

    NSBundle* extension_bundle = [NSBundle bundleWithPath:embedded_bundle_path];
    assert(extension_bundle != nil);
    NSDictionary* extension_info = extension_bundle.infoDictionary;
    assert(extension_info != nil);
    assert([extension_info[@"CFBundleDisplayName"]
        isEqualToString:@"CamelCrusher"]);

    NSDictionary* extension = extension_info[@"NSExtension"];
    assert(extension != nil);
    assert([extension[@"NSExtensionPointIdentifier"]
        isEqualToString:@"com.apple.AudioUnit-UI"]);
    assert([extension[@"NSExtensionPrincipalClass"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);

    NSDictionary* attributes = extension[@"NSExtensionAttributes"];
    assert(attributes != nil);
    NSArray* components = attributes[@"AudioComponents"];
    assert(components.count == 1U);
    NSDictionary* component = components.firstObject;
    assert([component[@"description"] isEqualToString:@"CamelCrusher"]);
    assert([component[@"factoryFunction"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);
    assert([component[@"name"] isEqualToString:@"Rivet: CamelCrusher"]);
    assert([component[@"version"] isEqual:@7]);

    NSString* executable_path = [embedded_bundle_path
        stringByAppendingPathComponent:@"Contents/MacOS/CamelCrusherAU"];
    assert([file_manager fileExistsAtPath:executable_path]);
    assert(hasExecutableMachOType(executable_path, MH_EXECUTE));
  }

  return 0;
}
