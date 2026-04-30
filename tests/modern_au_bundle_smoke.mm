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
    NSFileManager* file_manager = [NSFileManager defaultManager];
    NSString* bundle_path = @CAMELCRUSHER_AU_BUNDLE_PATH;
    NSBundle* bundle = [NSBundle bundleWithPath:bundle_path];
    assert(bundle != nil);
    assert([file_manager fileExistsAtPath:bundle_path]);

    NSDictionary* info = bundle.infoDictionary;
    assert(info != nil);
    assert([info[@"CFBundlePackageType"] isEqualToString:@"XPC!"]);
    assert([info[@"CFBundleDisplayName"] isEqualToString:@"CamelCrusher"]);

    NSDictionary* extension = info[@"NSExtension"];
    assert(extension != nil);
    assert([extension[@"NSExtensionPointIdentifier"]
        isEqualToString:@"com.apple.AudioUnit-UI"]);
    assert([extension[@"NSExtensionPrincipalClass"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);

    NSDictionary* attributes = extension[@"NSExtensionAttributes"];
    assert(attributes != nil);
    NSArray* components = attributes[@"AudioComponents"];
    assert(components != nil);
    assert(components.count == 1U);
    NSDictionary* component = components.firstObject;
    assert([component[@"description"] isEqualToString:@"CamelCrusher"]);
    assert([component[@"manufacturer"] isEqualToString:@"RvFx"]);
    assert([component[@"factoryFunction"]
        isEqualToString:@"CamelCrusherRecalledViewController"]);
    assert([component[@"name"] isEqualToString:@"Rivet: CamelCrusher"]);
    assert([component[@"subtype"] isEqualToString:@"CcrR"]);
    assert([component[@"type"] isEqualToString:@"aufx"]);
    assert([component[@"version"] isEqual:@7]);

    NSString* executable_path = [bundle_path
        stringByAppendingPathComponent:@"Contents/MacOS/CamelCrusherAU"];
    assert([file_manager fileExistsAtPath:executable_path]);
    assert(hasExecutableMachOType(executable_path, MH_EXECUTE));
  }

  return 0;
}
