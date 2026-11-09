#import "camelcrusher_recalled/CamelCrusherRecalledAudioUnit.h"
#import <AVFoundation/AVFoundation.h>

#include "camelcrusher_recalled/LegacyChunk.h"
#include "camelcrusher_recalled/LegacyImport.h"
#include "camelcrusher_recalled/ModernPluginModel.h"
#include "camelcrusher_recalled/ModernRuntime.h"

#import <AudioToolbox/AudioToolbox.h>
#import <CoreAudioKit/AUViewController.h>
#import <Foundation/Foundation.h>

#include <algorithm>
#include "TestSupport.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <dispatch/dispatch.h>
#include <vector>

namespace {

using camelcrusher_recalled::encodeLegacyChunk;
using camelcrusher_recalled::importLegacyState;
using camelcrusher_recalled::modernPluginStateFromLegacyImport;
using camelcrusher_recalled::serializeModernPluginState;
using camelcrusher_recalled::LegacyChunk;
using camelcrusher_recalled::LegacyProgram;

LegacyProgram makeProgram(const std::string& name, const float seed) {
  LegacyProgram program;
  program.record_marker = 1.0F;
  program.name = name;
  for (std::size_t index = 0; index < program.parameter_values.size(); ++index) {
    program.parameter_values[index] =
        seed + static_cast<float>(index) * 0.03125F;
  }
  return program;
}

bool nearlyEqual(const float left, const float right,
                 const float epsilon = 0.000001F) {
  return std::fabs(left - right) < epsilon;
}

float peakMagnitude(const std::vector<float>& samples) {
  float peak = 0.0F;
  for (const auto sample : samples) {
    peak = std::max(peak, std::fabs(sample));
  }
  return peak;
}

struct StereoBufferList {
  AudioBufferList list;
  AudioBuffer second;
};

struct InterleavedStereoBufferList {
  AudioBufferList list;
};

}  // namespace

int main() {
  @autoreleasepool {
    [CamelCrusherRecalledAudioUnit registerSubclass];

    NSError* error = nil;
    CamelCrusherRecalledAudioUnit* unit =
        [[CamelCrusherRecalledAudioUnit alloc]
            initWithComponentDescription:CamelCrusherRecalledAudioUnit.componentDescription
                                 options:kAudioComponentInstantiation_LoadInProcess
                                   error:&error];
    CC_CHECK(unit != nil);
    CC_CHECK(error == nil);

    CC_CHECK(unit.parameterTree != nil);
    CC_CHECK(unit.parameterTree.allParameters.count == 12U);
    CC_CHECK([[unit parametersForOverviewWithCount:4] count] == 4U);
    CC_CHECK(unit.channelCapabilities.count == 2U);
    CC_CHECK(unit.supportsUserPresets);
    CC_CHECK(unit.providesUserInterface);
    CC_CHECK(unit.factoryPresets.count == 20U);
    CC_CHECK(unit.currentPreset == nil);
    CC_CHECK([unit.factoryPresets.firstObject.name isEqualToString:@"Annihilate"]);
    CC_CHECK([unit.factoryPresets.lastObject.name isEqualToString:@"Turn it to 11"]);

    AUParameter* dist_on = [unit.parameterTree parameterWithAddress:0];
    AUParameter* master_mix = [unit.parameterTree parameterWithAddress:10];
    AUParameter* master_volume = [unit.parameterTree parameterWithAddress:11];
    CC_CHECK(dist_on != nil);
    CC_CHECK(master_mix != nil);
    CC_CHECK(master_volume != nil);

    dist_on.value = 1.0F;
    master_mix.value = 0.75F;
    master_volume.value = 0.60F;
    CC_CHECK(nearlyEqual(dist_on.value, 1.0F));
    CC_CHECK(nearlyEqual(master_mix.value, 0.75F));

    __block AUViewControllerBase* requested_view_controller = nil;
    dispatch_semaphore_t view_controller_semaphore = dispatch_semaphore_create(0);
    [unit requestViewControllerWithCompletionHandler:^(
              AUViewControllerBase* _Nullable viewController) {
      requested_view_controller = viewController;
      dispatch_semaphore_signal(view_controller_semaphore);
    }];
    CC_CHECK(dispatch_semaphore_wait(view_controller_semaphore,
                                   dispatch_time(DISPATCH_TIME_NOW,
                                                 2 * NSEC_PER_SEC)) == 0);
    CC_CHECK(requested_view_controller != nil);
    [requested_view_controller loadView];
    CC_CHECK(requested_view_controller.view != nil);
    CC_CHECK(requested_view_controller.view.frame.size.width > 0.0);
    CC_CHECK(requested_view_controller.view.frame.size.height > 0.0);

    NSDictionary<NSString*, id>* default_state = unit.fullState;
    NSData* default_blob = default_state[CamelCrusherRecalledAUStateBlobKey];
    CC_CHECK(default_blob != nil);
    CC_CHECK(default_blob.length > 0U);
    const AudioComponentDescription component_description =
        CamelCrusherRecalledAudioUnit.componentDescription;
    CC_CHECK([default_state[CamelCrusherRecalledAUClassInfoTypeKey]
        isEqual:@(component_description.componentType)]);
    CC_CHECK([default_state[CamelCrusherRecalledAUClassInfoSubtypeKey]
        isEqual:@(component_description.componentSubType)]);
    CC_CHECK([default_state[CamelCrusherRecalledAUClassInfoManufacturerKey]
        isEqual:@(component_description.componentManufacturer)]);
    CC_CHECK([default_state[CamelCrusherRecalledAUClassInfoPresetNumberKey]
        isEqual:@(-1)]);
    CC_CHECK([default_state[CamelCrusherRecalledAUClassInfoNameKey]
        isEqualToString:@"Current State"]);

    LegacyChunk synthetic_chunk;
    synthetic_chunk.programs.push_back(makeProgram("Neutral", 0.10F));
    synthetic_chunk.programs.push_back(makeProgram("Driven", 0.20F));
    const auto encoded = encodeLegacyChunk(synthetic_chunk);
    const std::vector<float> explicit_values(
        synthetic_chunk.programs[1].parameter_values.begin(),
        synthetic_chunk.programs[1].parameter_values.end());
    const auto imported =
        importLegacyState(synthetic_chunk, 1, explicit_values);
    const auto serialized =
        serializeModernPluginState(modernPluginStateFromLegacyImport(imported));
    NSData* imported_blob =
        [NSData dataWithBytes:serialized.data() length:serialized.size()];
    unit.fullState = @{CamelCrusherRecalledAUStateBlobKey : imported_blob};

    CC_CHECK(unit.factoryPresets.count == 2U);
    CC_CHECK(unit.currentPreset != nil);
    CC_CHECK(unit.currentPreset.number == 1);
    CC_CHECK([unit.currentPreset.name isEqualToString:@"Driven"]);

    NSDictionary<NSString*, id>* imported_state = unit.fullState;
    CC_CHECK([imported_state[CamelCrusherRecalledAUClassInfoPresetNumberKey]
        isEqual:@1]);
    CC_CHECK([imported_state[CamelCrusherRecalledAUClassInfoNameKey]
        isEqualToString:@"Driven"]);

    AUAudioUnitPreset* neutral_preset = unit.factoryPresets.firstObject;
    unit.currentPreset = neutral_preset;
    CC_CHECK(unit.currentPreset.number == 0);
    CC_CHECK([unit.currentPreset.name isEqualToString:@"Neutral"]);
    CC_CHECK(nearlyEqual(master_mix.value,
                       synthetic_chunk.programs[0].parameter_values[10]));

    unit.fullState = @{
      CamelCrusherRecalledAUClassInfoDataKey : imported_blob,
      CamelCrusherRecalledAUClassInfoNameKey : @"Ableton Import",
      CamelCrusherRecalledAUClassInfoPresetNumberKey : @(-19),
    };
    CC_CHECK(unit.currentPreset != nil);
    CC_CHECK(unit.currentPreset.number == -19);
    CC_CHECK([unit.currentPreset.name isEqualToString:@"Ableton Import"]);
    NSDictionary<NSString*, id>* user_preset_state = unit.fullState;
    CC_CHECK([user_preset_state[CamelCrusherRecalledAUClassInfoPresetNumberKey]
        isEqual:@(-19)]);
    CC_CHECK([user_preset_state[CamelCrusherRecalledAUClassInfoNameKey]
        isEqualToString:@"Ableton Import"]);

    NSString* preset_dir = [NSTemporaryDirectory()
        stringByAppendingPathComponent:[[NSUUID UUID] UUIDString]];
    setenv("CAMELCRUSHER_AU_USER_PRESET_DIR", preset_dir.UTF8String, 1);
    AUAudioUnitPreset* saved_user_preset = [AUAudioUnitPreset new];
    saved_user_preset.number = -23;
    saved_user_preset.name = @"Smoke Imported";
    CC_CHECK([unit saveUserPreset:saved_user_preset error:&error]);
    CC_CHECK(error == nil);
    NSArray<AUAudioUnitPreset*>* user_presets = unit.userPresets;
    CC_CHECK(user_presets.count == 1U);
    CC_CHECK([user_presets.firstObject.name isEqualToString:@"Smoke Imported"]);
    NSDictionary<NSString*, id>* saved_preset_state =
        [unit presetStateFor:user_presets.firstObject error:&error];
    CC_CHECK(saved_preset_state != nil);
    CC_CHECK(error == nil);
    unit.currentPreset = user_presets.firstObject;
    CC_CHECK(unit.currentPreset.number == -23);
    CC_CHECK([unit.currentPreset.name isEqualToString:@"Smoke Imported"]);
    AUAudioUnitPreset* missing_user_preset = [AUAudioUnitPreset new];
    missing_user_preset.number = -99;
    missing_user_preset.name = @"Missing User Preset";
    NSDictionary<NSString*, id>* missing_preset_state =
        [unit presetStateFor:missing_user_preset error:&error];
    CC_CHECK(missing_preset_state == nil);
    CC_CHECK(error != nil);
    unsetenv("CAMELCRUSHER_AU_USER_PRESET_DIR");
    [[NSFileManager defaultManager] removeItemAtPath:preset_dir error:nil];

    dist_on.value = 1.0F;
    [unit.parameterTree parameterWithAddress:1].value = 0.85F;
    [unit.parameterTree parameterWithAddress:2].value = 0.70F;
    [unit.parameterTree parameterWithAddress:3].value = 1.0F;
    [unit.parameterTree parameterWithAddress:4].value = 0.20F;
    [unit.parameterTree parameterWithAddress:5].value = 0.55F;
    [unit.parameterTree parameterWithAddress:6].value = 1.0F;
    [unit.parameterTree parameterWithAddress:7].value = 0.75F;
    [unit.parameterTree parameterWithAddress:8].value = 0.35F;
    [unit.parameterTree parameterWithAddress:9].value = 1.0F;
    master_mix.value = 1.0F;
    master_volume.value = 0.82F;

    error = nil;
    unit.maximumFramesToRender = 32;
    CC_CHECK([unit allocateRenderResourcesAndReturnError:&error]);
    CC_CHECK(error == nil);

    AURenderBlock render = unit.renderBlock;
    CC_CHECK(render != nil);

    std::vector<float> input_left(16, 0.0F);
    std::vector<float> input_right(16, 0.0F);
    input_left[0] = 0.7F;
    input_right[0] = 0.7F;

    std::vector<float> output_left(16, 0.0F);
    std::vector<float> output_right(16, 0.0F);

    StereoBufferList output{};
    output.list.mNumberBuffers = 2;
    output.list.mBuffers[0].mNumberChannels = 1;
    output.list.mBuffers[0].mDataByteSize =
        static_cast<UInt32>(output_left.size() * sizeof(float));
    output.list.mBuffers[0].mData = output_left.data();
    output.list.mBuffers[1].mNumberChannels = 1;
    output.list.mBuffers[1].mDataByteSize =
        static_cast<UInt32>(output_right.size() * sizeof(float));
    output.list.mBuffers[1].mData = output_right.data();

    AudioTimeStamp timestamp{};
    timestamp.mFlags = kAudioTimeStampSampleTimeValid;
    timestamp.mSampleTime = 0;
    AudioUnitRenderActionFlags flags = 0;

    AURenderPullInputBlock pull_input =
        ^AUAudioUnitStatus(AudioUnitRenderActionFlags* action_flags,
                           const AudioTimeStamp* pull_timestamp,
                           AUAudioFrameCount frame_count,
                           NSInteger input_bus_number,
                           AudioBufferList* input_data) {
          (void)action_flags;
          (void)pull_timestamp;
          (void)input_bus_number;
          std::memcpy(input_data->mBuffers[0].mData, input_left.data(),
                      frame_count * sizeof(float));
          std::memcpy(input_data->mBuffers[1].mData, input_right.data(),
                      frame_count * sizeof(float));
          return noErr;
        };

    const auto render_status = render(&flags, &timestamp, 16, 0, &output.list,
                                      pull_input);
    CC_CHECK(render_status == noErr);
    CC_CHECK(!nearlyEqual(output_left[0], 0.7F));
    CC_CHECK(!nearlyEqual(output_right[0], 0.7F));
    CC_CHECK(peakMagnitude(output_left) <= 1.0F);
    CC_CHECK(peakMagnitude(output_right) <= 1.0F);

    [unit reset];
    std::fill(input_left.begin(), input_left.end(), 0.0F);
    std::fill(input_right.begin(), input_right.end(), 0.0F);
    std::fill(output_left.begin(), output_left.end(), 0.0F);
    std::fill(output_right.begin(), output_right.end(), 0.0F);
    input_right[0] = 0.65F;
    input_right[2] = -0.30F;
    AURenderPullInputBlock right_only_pull_input =
        ^AUAudioUnitStatus(AudioUnitRenderActionFlags* action_flags,
                           const AudioTimeStamp* pull_timestamp,
                           AUAudioFrameCount frame_count,
                           NSInteger input_bus_number,
                           AudioBufferList* input_data) {
          (void)action_flags;
          (void)pull_timestamp;
          (void)input_bus_number;
          std::memcpy(input_data->mBuffers[0].mData, input_left.data(),
                      frame_count * sizeof(float));
          std::memcpy(input_data->mBuffers[1].mData, input_right.data(),
                      frame_count * sizeof(float));
          return noErr;
        };

    const auto right_only_status = render(&flags, &timestamp, 16, 0,
                                          &output.list, right_only_pull_input);
    CC_CHECK(right_only_status == noErr);
    CC_CHECK(nearlyEqual(output_left[0], 0.0F, 0.000001F));
    CC_CHECK(nearlyEqual(output_left[2], 0.0F, 0.000001F));
    CC_CHECK(!nearlyEqual(output_right[0], 0.0F));
    CC_CHECK(!nearlyEqual(output_right[2], 0.0F));

    [unit deallocateRenderResources];
    AVAudioFormat* interleaved_format = [[AVAudioFormat alloc]
        initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:44100.0
        channels:2 interleaved:YES];
    CC_CHECK(![unit.inputBusses[0] setFormat:interleaved_format error:&error]);
    CC_CHECK(error.code == kAudioUnitErr_FormatNotSupported);
    error = nil;
    CC_CHECK(![unit.outputBusses[0] setFormat:interleaved_format error:&error]);
    CC_CHECK(error.code == kAudioUnitErr_FormatNotSupported);
    [unit deallocateRenderResources];
  }

  return 0;
}
