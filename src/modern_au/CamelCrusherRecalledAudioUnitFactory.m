#import "camelcrusher_recalled/CamelCrusherRecalledAudioUnit.h"

#import <AudioToolbox/AUAudioUnitImplementation.h>

@interface CamelCrusherRecalledAudioUnitFactory : NSObject <AUAudioUnitFactory>
@end

@implementation CamelCrusherRecalledAudioUnitFactory

- (void)beginRequestWithExtensionContext:(NSExtensionContext*)context {
  (void)context;
}

- (AUAudioUnit*)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                                  error:(NSError* _Nullable __autoreleasing*)error {
  return [[CamelCrusherRecalledAudioUnit alloc] initWithComponentDescription:desc
                                                                     options:0
                                                                       error:error];
}

@end
