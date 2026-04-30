#pragma once

#if defined(__APPLE__)

#import <AudioToolbox/AUAudioUnitImplementation.h>
#import <CoreAudioKit/AUViewController.h>

@class CamelCrusherRecalledAudioUnit;

NS_ASSUME_NONNULL_BEGIN

@interface CamelCrusherRecalledViewController : AUViewController <AUAudioUnitFactory>

- (instancetype)initWithAudioUnit:(CamelCrusherRecalledAudioUnit*)audioUnit;

@end

NS_ASSUME_NONNULL_END

#endif  // defined(__APPLE__)
