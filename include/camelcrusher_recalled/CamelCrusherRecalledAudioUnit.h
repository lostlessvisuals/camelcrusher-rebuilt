#pragma once

#if defined(__APPLE__)

#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUStateBlobKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoDataKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoTypeKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoSubtypeKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoManufacturerKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoVersionKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoNameKey;
FOUNDATION_EXPORT NSString* const CamelCrusherRecalledAUClassInfoPresetNumberKey;

@interface CamelCrusherRecalledAudioUnit : AUAudioUnit

+ (AudioComponentDescription)componentDescription;
+ (void)registerSubclass;

@end

NS_ASSUME_NONNULL_END

#endif  // defined(__APPLE__)
