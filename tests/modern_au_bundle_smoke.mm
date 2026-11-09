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
    NSFileManager* file_manager = [NSFileManager defaultManager];
    NSString* bundle_path = @CAMELCRUSHER_AU_BUNDLE_PATH;
    NSBundle* bundle = [NSBundle bundleWithPath:bundle_path];
    CC_CHECK(bundle != nil);
    CC_CHECK([file_manager fileExistsAtPath:bundle_path]);

    NSDictionary* info = bundle.infoDictionary;
    CC_CHECK(info != nil);
    CC_CHECK([info[@"CFBundlePackageType"] isEqualToString:@"XPC!"]);
    CC_CHECK([info[@"CFBundleDisplayName"] isEqualToString:@"CamelCrusher"]);

    NSDictionary* extension = info[@"NSExtension"];
    CC_CHECK(extension != nil);
    CC_CHECK([extension[@"NSExtensionPointIdentifier"]
        isEqualToString:@"com.apple.AudioUnit-UI"]);
    CC_CHECK([extension[@"NSExtensionPrincipalClass"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);

    NSDictionary* attributes = extension[@"NSExtensionAttributes"];
    CC_CHECK(attributes != nil);
    NSArray* components = attributes[@"AudioComponents"];
    CC_CHECK(components != nil);
    CC_CHECK(components.count == 1U);
    NSDictionary* component = components.firstObject;
    CC_CHECK([component[@"description"] isEqualToString:@"CamelCrusher"]);
    CC_CHECK([component[@"manufacturer"] isEqualToString:@"CmAu"]);
    CC_CHECK([component[@"factoryFunction"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);
    CC_CHECK([component[@"name"] isEqualToString:@"Camel Audio: CamelCrusher"]);
    CC_CHECK([component[@"subtype"] isEqualToString:@"CcrR"]);
    CC_CHECK([component[@"type"] isEqualToString:@"aufx"]);
    CC_CHECK([component[@"version"] isEqual:@(CAMELCRUSHER_EXPECTED_BUILD)]);

    NSString* executable_path = [bundle_path
        stringByAppendingPathComponent:@"Contents/MacOS/CamelCrusherAU"];
    CC_CHECK([file_manager fileExistsAtPath:executable_path]);
    CC_CHECK(hasExecutableMachOType(executable_path, MH_EXECUTE));
  }

  return 0;
}
