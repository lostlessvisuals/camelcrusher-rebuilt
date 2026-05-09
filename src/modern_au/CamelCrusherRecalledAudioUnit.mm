#import "camelcrusher_recalled/CamelCrusherRecalledAudioUnit.h"
#import "CamelCrusherRecalledViewController.h"

#import "camelcrusher_recalled/ModernRuntimeBridge.h"

#import <AudioToolbox/AUAudioUnitImplementation.h>
#import <AVFoundation/AVFoundation.h>

#include <algorithm>
#include <vector>

#ifndef CAMELCRUSHER_AU_COMPONENT_VERSION_INT
#define CAMELCRUSHER_AU_COMPONENT_VERSION_INT 8
#endif

NSString* const CamelCrusherRecalledAUStateBlobKey =
    @"camelcrusher_runtime_state";
NSString* const CamelCrusherRecalledAUClassInfoDataKey = @kAUPresetDataKey;
NSString* const CamelCrusherRecalledAUClassInfoTypeKey = @kAUPresetTypeKey;
NSString* const CamelCrusherRecalledAUClassInfoSubtypeKey = @kAUPresetSubtypeKey;
NSString* const CamelCrusherRecalledAUClassInfoManufacturerKey =
    @kAUPresetManufacturerKey;
NSString* const CamelCrusherRecalledAUClassInfoVersionKey = @kAUPresetVersionKey;
NSString* const CamelCrusherRecalledAUClassInfoNameKey = @kAUPresetNameKey;
NSString* const CamelCrusherRecalledAUClassInfoPresetNumberKey =
    @kAUPresetNumberKey;

namespace {

constexpr OSType fourCC(const char a,
                        const char b,
                        const char c,
                        const char d) {
  return (static_cast<OSType>(a) << 24U) | (static_cast<OSType>(b) << 16U) |
         (static_cast<OSType>(c) << 8U) | static_cast<OSType>(d);
}

AudioUnitParameterUnit parameterUnitForDescriptor(
    const CamelCrusherRuntimeBridgePublicParameterDescriptor& descriptor) {
  return descriptor.value_kind ==
                 CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_SWITCH_LIKE
             ? kAudioUnitParameterUnit_Boolean
             : kAudioUnitParameterUnit_Generic;
}

AudioUnitParameterOptions parameterFlagsForDescriptor(
    const CamelCrusherRuntimeBridgePublicParameterDescriptor& descriptor) {
  AudioUnitParameterOptions flags =
      kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;
  if (descriptor.value_kind != CAMELCRUSHER_RUNTIME_BRIDGE_VALUE_SWITCH_LIKE) {
    flags |= kAudioUnitParameterFlag_CanRamp;
  }
  return flags;
}

NSError* makeRuntimeBridgeError(const CamelCrusherRuntimeBridgeTextError& error) {
  NSString* description = [NSString stringWithUTF8String:error.message];
  NSDictionary* user_info = @{
    NSLocalizedDescriptionKey : description ?: @"Runtime bridge error"
  };
  return [NSError errorWithDomain:@"CamelCrusherRecalledAudioUnitErrorDomain"
                             code:static_cast<NSInteger>(error.offset)
                         userInfo:user_info];
}

NSData* runtimeStateData(CamelCrusherRuntimeBridgeRuntime* runtime) {
  const auto byte_count = camelcrusher_runtime_bridge_runtime_state_size(runtime);
  if (byte_count == 0U) {
    return [NSData data];
  }

  NSMutableData* data = [NSMutableData dataWithLength:byte_count];
  size_t bytes_written = 0;
  if (!camelcrusher_runtime_bridge_runtime_save_state(
          runtime, static_cast<uint8_t*>(data.mutableBytes), byte_count,
          &bytes_written)) {
    return [NSData data];
  }
  if (bytes_written != byte_count) {
    [data setLength:bytes_written];
  }
  return data;
}

AUAudioUnitPreset* makePreset(const NSInteger number, NSString* name) {
  AUAudioUnitPreset* preset = [AUAudioUnitPreset new];
  preset.number = number;
  preset.name = name ?: @"Current State";
  return preset;
}

NSDictionary<NSString*, id>* normalizedClassInfoState(
    NSDictionary<NSString*, id>* _Nullable state,
    const AudioComponentDescription description,
    NSData* fallback_data,
    NSString* fallback_name,
    const NSInteger fallback_preset_number) {
  NSMutableDictionary<NSString*, id>* normalized = [NSMutableDictionary dictionary];
  if (state != nil) {
    [normalized addEntriesFromDictionary:state];
  }

  NSData* data = nil;
  if ([normalized[CamelCrusherRecalledAUStateBlobKey] isKindOfClass:[NSData class]]) {
    data = normalized[CamelCrusherRecalledAUStateBlobKey];
  } else if ([normalized[CamelCrusherRecalledAUClassInfoDataKey]
                 isKindOfClass:[NSData class]]) {
    data = normalized[CamelCrusherRecalledAUClassInfoDataKey];
  } else {
    data = fallback_data ?: [NSData data];
  }

  NSString* name = [normalized[CamelCrusherRecalledAUClassInfoNameKey]
      isKindOfClass:[NSString class]]
                       ? normalized[CamelCrusherRecalledAUClassInfoNameKey]
                       : fallback_name;
  NSNumber* preset_number = [normalized[CamelCrusherRecalledAUClassInfoPresetNumberKey]
      isKindOfClass:[NSNumber class]]
                                 ? normalized[CamelCrusherRecalledAUClassInfoPresetNumberKey]
                                 : @(fallback_preset_number);

  normalized[CamelCrusherRecalledAUClassInfoTypeKey] =
      @(static_cast<unsigned int>(description.componentType));
  normalized[CamelCrusherRecalledAUClassInfoSubtypeKey] =
      @(static_cast<unsigned int>(description.componentSubType));
  normalized[CamelCrusherRecalledAUClassInfoManufacturerKey] =
      @(static_cast<unsigned int>(description.componentManufacturer));
  normalized[CamelCrusherRecalledAUClassInfoVersionKey] =
      @(CAMELCRUSHER_AU_COMPONENT_VERSION_INT);
  normalized[CamelCrusherRecalledAUClassInfoNameKey] =
      name ?: @"Current State";
  normalized[CamelCrusherRecalledAUClassInfoPresetNumberKey] = preset_number;
  normalized[CamelCrusherRecalledAUClassInfoDataKey] = data;
  normalized[CamelCrusherRecalledAUStateBlobKey] = data;
  return normalized;
}

AUAudioUnitPreset* userPresetFromStateDictionary(
    NSDictionary<NSString*, id>* _Nullable state) {
  if (![state[CamelCrusherRecalledAUClassInfoPresetNumberKey]
          isKindOfClass:[NSNumber class]]) {
    return nil;
  }

  const NSInteger preset_number =
      [state[CamelCrusherRecalledAUClassInfoPresetNumberKey] integerValue];
  if (preset_number >= 0) {
    return nil;
  }

  NSString* preset_name = [state[CamelCrusherRecalledAUClassInfoNameKey]
      isKindOfClass:[NSString class]]
                               ? state[CamelCrusherRecalledAUClassInfoNameKey]
                               : @"Imported Preset";
  return makePreset(preset_number, preset_name);
}

NSString* configuredUserPresetDirectoryPath() {
  NSString* override_path =
      [NSProcessInfo processInfo].environment[@"CAMELCRUSHER_AU_USER_PRESET_DIR"];
  if (override_path.length > 0U) {
    return override_path;
  }
  return [NSHomeDirectory()
      stringByAppendingPathComponent:
          @"Library/Audio/Presets/Camel Audio/CamelCrusher"];
}

NSString* sanitizedPresetFilename(NSString* name) {
  NSString* trimmed =
      [[name stringByTrimmingCharactersInSet:
                 [NSCharacterSet whitespaceAndNewlineCharacterSet]] copy];
  NSString* normalized = trimmed.length > 0U ? trimmed : @"Imported Preset";
  return [normalized stringByReplacingOccurrencesOfString:@"/"
                                               withString:@"-"];
}

NSURL* userPresetDirectoryURL() {
  return [NSURL fileURLWithPath:configuredUserPresetDirectoryPath()
                    isDirectory:YES];
}

NSURL* userPresetFileURL(NSString* name) {
  NSString* filename =
      [sanitizedPresetFilename(name) stringByAppendingPathExtension:@"aupreset"];
  if (filename.length == 0U) {
    filename = @"Imported Preset.aupreset";
  }
  return [userPresetDirectoryURL()
      URLByAppendingPathComponent:filename];
}

NSDictionary<NSString*, id>* loadPresetDictionaryFromURL(NSURL* preset_url,
                                                         NSError** outError) {
  if (preset_url == nil || !preset_url.fileURL || preset_url.path.length == 0U) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:NSOSStatusErrorDomain
                                      code:kAudioUnitErr_InvalidFilePath
                                  userInfo:@{
                                    NSLocalizedDescriptionKey :
                                        @"User preset path was invalid"
                                  }];
    }
    return nil;
  }

  if (![[NSFileManager defaultManager] fileExistsAtPath:preset_url.path]) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:NSCocoaErrorDomain
                                      code:NSFileNoSuchFileError
                                  userInfo:@{
                                    NSFilePathErrorKey : preset_url.path,
                                    NSLocalizedDescriptionKey :
                                        @"User preset file was not found"
                                  }];
    }
    return nil;
  }

  NSData* data = nil;
  @try {
    data = [NSData dataWithContentsOfURL:preset_url
                                 options:0
                                   error:outError];
  } @catch (NSException* exception) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:NSCocoaErrorDomain
                                      code:NSFileReadUnknownError
                                  userInfo:@{
                                    NSLocalizedDescriptionKey :
                                        exception.reason ?: @"User preset read threw an exception"
                                  }];
    }
    return nil;
  }

  if (data == nil) {
    return nil;
  }

  id property_list =
      [NSPropertyListSerialization propertyListWithData:data
                                                options:NSPropertyListImmutable
                                                 format:nil
                                                  error:outError];
  if (![property_list isKindOfClass:[NSDictionary class]]) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:NSCocoaErrorDomain
                                      code:NSFileReadCorruptFileError
                                  userInfo:@{
                                    NSLocalizedDescriptionKey :
                                        @"AU preset file did not contain a dictionary"
                                  }];
    }
    return nil;
  }
  return property_list;
}

BOOL writePresetDictionaryToURL(NSDictionary<NSString*, id>* preset_state,
                                NSURL* preset_url,
                                NSError** outError) {
  NSError* serialization_error = nil;
  NSData* data = [NSPropertyListSerialization
      dataWithPropertyList:preset_state
                    format:NSPropertyListBinaryFormat_v1_0
                   options:0
                     error:&serialization_error];
  if (data == nil) {
    if (outError != nullptr) {
      *outError = serialization_error;
    }
    return NO;
  }

  NSFileManager* file_manager = [NSFileManager defaultManager];
  NSURL* directory_url = [preset_url URLByDeletingLastPathComponent];
  if (![file_manager createDirectoryAtURL:directory_url
              withIntermediateDirectories:YES
                               attributes:nil
                                    error:outError]) {
    return NO;
  }

  return [data writeToURL:preset_url options:NSDataWritingAtomic error:outError];
}

NSInteger normalizedUserPresetNumber(const NSInteger preset_number) {
  if (preset_number > 0) {
    return -preset_number;
  }
  if (preset_number == 0) {
    return -1;
  }
  return preset_number;
}

}  // namespace

@interface CamelCrusherRecalledAudioUnit () {
  CamelCrusherRuntimeBridgeRuntime* _runtime;
  AUAudioUnitBus* _inputBus;
  AUAudioUnitBus* _outputBus;
  AUAudioUnitBusArray* _inputBusArray;
  AUAudioUnitBusArray* _outputBusArray;
  AUParameterTree* _parameterTree;
  NSArray<AUAudioUnitPreset*>* _factoryPresets;
  AUAudioUnitPreset* _currentUserPreset;
  CamelCrusherRecalledViewController* _viewController;
  std::vector<float> _interleavedScratchLeft;
  std::vector<float> _interleavedScratchRight;
}

- (AUParameterTree*)buildParameterTree;
- (NSInteger)currentStatePresetNumber;
- (NSString*)currentStatePresetName;
- (void)refreshFactoryPresets;
- (void)syncParameterValuesFromRuntime;
- (BOOL)loadRuntimeStateData:(NSData*)data error:(NSError**)outError;

@end

@implementation CamelCrusherRecalledAudioUnit

+ (AudioComponentDescription)componentDescription {
  AudioComponentDescription description{};
  description.componentType = kAudioUnitType_Effect;
  description.componentSubType = fourCC('C', 'c', 'r', 'R');
  description.componentManufacturer = fourCC('C', 'm', 'A', 'u');
  description.componentFlags = 0;
  description.componentFlagsMask = 0;
  return description;
}

+ (void)registerSubclass {
  static dispatch_once_t once_token;
  dispatch_once(&once_token, ^{
    [AUAudioUnit registerSubclass:self
           asComponentDescription:self.componentDescription
                             name:@"Camel Audio: CamelCrusher"
                          version:CAMELCRUSHER_AU_COMPONENT_VERSION_INT];
  });
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError* _Nullable __autoreleasing*)outError {
  self = [super initWithComponentDescription:componentDescription
                                     options:options
                                       error:outError];
  if (self == nil) {
    return nil;
  }

  _runtime = camelcrusher_runtime_bridge_runtime_create();
  if (_runtime == nullptr) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:@"CamelCrusherRecalledAudioUnitErrorDomain"
                                      code:-1
                                  userInfo:@{
                                    NSLocalizedDescriptionKey :
                                        @"Unable to create CamelCrusher runtime"
                                  }];
    }
    return nil;
  }

  AVAudioFormat* stereo_format =
      [[AVAudioFormat alloc] initStandardFormatWithSampleRate:44100.0
                                                     channels:2];
  _inputBus = [[AUAudioUnitBus alloc] initWithFormat:stereo_format error:outError];
  if (_inputBus == nil) {
    return nil;
  }

  _outputBus = [[AUAudioUnitBus alloc] initWithFormat:stereo_format error:outError];
  if (_outputBus == nil) {
    return nil;
  }

  _inputBus.shouldAllocateBuffer = NO;
  _outputBus.shouldAllocateBuffer = NO;

  _inputBusArray =
      [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                             busType:AUAudioUnitBusTypeInput
                                              busses:@[ _inputBus ]];
  _outputBusArray =
      [[AUAudioUnitBusArray alloc] initWithAudioUnit:self
                                             busType:AUAudioUnitBusTypeOutput
                                              busses:@[ _outputBus ]];
  _parameterTree = [self buildParameterTree];
  self.parameterTree = _parameterTree;

  self.maximumFramesToRender = 4096;
  [self refreshFactoryPresets];
  [self syncParameterValuesFromRuntime];
  return self;
}

- (void)dealloc {
  if (_runtime != nullptr) {
    camelcrusher_runtime_bridge_runtime_destroy(_runtime);
    _runtime = nullptr;
  }
}

- (AUAudioUnitBusArray*)inputBusses {
  return _inputBusArray;
}

- (AUAudioUnitBusArray*)outputBusses {
  return _outputBusArray;
}

- (NSArray<NSNumber*>*)channelCapabilities {
  return @[ @2, @2 ];
}

- (void)requestViewControllerWithCompletionHandler:
    (void (^)(AUViewControllerBase* _Nullable viewController))completionHandler {
  if (completionHandler == nil) {
    return;
  }

  if (_viewController == nil) {
    _viewController =
        [[CamelCrusherRecalledViewController alloc] initWithAudioUnit:self];
  }
  completionHandler(_viewController);
}

- (NSIndexSet*)supportedViewConfigurations:
    (NSArray<AUAudioUnitViewConfiguration*>*)availableViewConfigurations {
  NSMutableIndexSet* supported = [NSMutableIndexSet indexSet];
  [availableViewConfigurations enumerateObjectsUsingBlock:^(
                                   AUAudioUnitViewConfiguration* configuration,
                                   NSUInteger index,
                                   BOOL* stop) {
    (void)stop;
    const bool has_room =
        configuration.width == 0.0 || configuration.height == 0.0 ||
        (configuration.width >= 345.0 && configuration.height >= 373.0);
    if (!configuration.hostHasController && has_room) {
      [supported addIndex:index];
    }
  }];
  return supported;
}

- (void)selectViewConfiguration:(AUAudioUnitViewConfiguration*)viewConfiguration {
  (void)viewConfiguration;
}

- (AUParameterTree*)parameterTree {
  if (_parameterTree == nil) {
    _parameterTree = [self buildParameterTree];
    self.parameterTree = _parameterTree;
  }
  return _parameterTree;
}

- (NSArray<NSNumber*>*)parametersForOverviewWithCount:(NSInteger)count {
  const auto parameter_count = camelcrusher_runtime_bridge_public_parameter_count();
  const auto bounded_count = count < 0 ? 0 : static_cast<size_t>(count);
  const auto clamped_count = std::min(parameter_count, bounded_count);
  NSMutableArray<NSNumber*>* overview =
      [NSMutableArray arrayWithCapacity:clamped_count];
  for (size_t index = 0; index < clamped_count; ++index) {
    [overview addObject:@(index)];
  }
  return overview;
}

- (BOOL)shouldChangeToFormat:(AVAudioFormat*)format forBus:(AUAudioUnitBus*)bus {
  if (![super shouldChangeToFormat:format forBus:bus]) {
    return NO;
  }
  return format.channelCount == 2 &&
         format.commonFormat == AVAudioPCMFormatFloat32;
}

- (BOOL)allocateRenderResourcesAndReturnError:
    (NSError* _Nullable __autoreleasing*)outError {
  if (_inputBus.format.channelCount != 2 || _outputBus.format.channelCount != 2 ||
      _inputBus.format.sampleRate != _outputBus.format.sampleRate) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:@"CamelCrusherRecalledAudioUnitErrorDomain"
                                      code:-2
                                  userInfo:@{
                                    NSLocalizedDescriptionKey :
                                        @"Input and output busses must stay stereo and aligned"
                                  }];
    }
    return NO;
  }

  const auto maximum_frames = static_cast<size_t>(self.maximumFramesToRender);
  camelcrusher_runtime_bridge_runtime_prepare(
      _runtime,
      CamelCrusherRuntimeBridgeProcessSpec{
          .sample_rate = _outputBus.format.sampleRate,
          .max_block_size = maximum_frames,
          .channel_count = _outputBus.format.channelCount,
      });
  _interleavedScratchLeft.assign(maximum_frames, 0.0F);
  _interleavedScratchRight.assign(maximum_frames, 0.0F);

  if (![super allocateRenderResourcesAndReturnError:outError]) {
    return NO;
  }

  return YES;
}

- (void)deallocateRenderResources {
  camelcrusher_runtime_bridge_runtime_reset(_runtime);
  [super deallocateRenderResources];
}

- (void)reset {
  camelcrusher_runtime_bridge_runtime_reset(_runtime);
  [super reset];
}

- (AUInternalRenderBlock)internalRenderBlock {
  __block CamelCrusherRuntimeBridgeRuntime* runtime = _runtime;
  __block std::vector<float>* interleaved_left = &_interleavedScratchLeft;
  __block std::vector<float>* interleaved_right = &_interleavedScratchRight;
  return ^AUAudioUnitStatus(AudioUnitRenderActionFlags* actionFlags,
                            const AudioTimeStamp* timestamp,
                            AUAudioFrameCount frameCount,
                            NSInteger outputBusNumber,
                            AudioBufferList* outputData,
                            const AURenderEvent* realtimeEventListHead,
                            AURenderPullInputBlock pullInputBlock) {
    (void)outputBusNumber;
    (void)realtimeEventListHead;
    if (pullInputBlock == nil) {
      return kAudioUnitErr_NoConnection;
    }
    if (outputData == nullptr || outputData->mNumberBuffers == 0) {
      return kAudio_ParamError;
    }

    const auto status =
        pullInputBlock(actionFlags, timestamp, frameCount, 0, outputData);
    if (status != noErr) {
      return status;
    }

    float* left = nullptr;
    float* right = nullptr;

    if (outputData->mNumberBuffers >= 2 &&
        outputData->mBuffers[0].mNumberChannels == 1 &&
        outputData->mBuffers[1].mNumberChannels == 1) {
      left = static_cast<float*>(outputData->mBuffers[0].mData);
      right = static_cast<float*>(outputData->mBuffers[1].mData);
    } else if (outputData->mNumberBuffers == 1 &&
               outputData->mBuffers[0].mNumberChannels == 2) {
      if (frameCount > interleaved_left->size() ||
          frameCount > interleaved_right->size()) {
        return kAudioUnitErr_TooManyFramesToProcess;
      }

      auto* interleaved_samples =
          static_cast<float*>(outputData->mBuffers[0].mData);
      if (interleaved_samples == nullptr) {
        return kAudio_ParamError;
      }

      for (AUAudioFrameCount frame = 0; frame < frameCount; ++frame) {
        (*interleaved_left)[frame] = interleaved_samples[frame * 2];
        (*interleaved_right)[frame] = interleaved_samples[frame * 2 + 1];
      }

      left = interleaved_left->data();
      right = interleaved_right->data();
    }

    if (left == nullptr || right == nullptr) {
      return kAudio_ParamError;
    }

    camelcrusher_runtime_bridge_runtime_process_stereo(
        runtime, left, right, static_cast<size_t>(frameCount));

    if (outputData->mNumberBuffers == 1 &&
        outputData->mBuffers[0].mNumberChannels == 2) {
      auto* interleaved_samples =
          static_cast<float*>(outputData->mBuffers[0].mData);
      for (AUAudioFrameCount frame = 0; frame < frameCount; ++frame) {
        interleaved_samples[frame * 2] = (*interleaved_left)[frame];
        interleaved_samples[frame * 2 + 1] = (*interleaved_right)[frame];
      }
    }
    return noErr;
  };
}

- (NSDictionary<NSString*, id>*)fullState {
  return normalizedClassInfoState(self->_currentUserPreset == nil ? nil : @{
                                    CamelCrusherRecalledAUClassInfoNameKey :
                                        self->_currentUserPreset.name ?: @"Current State",
                                    CamelCrusherRecalledAUClassInfoPresetNumberKey :
                                        @(self->_currentUserPreset.number),
                                  },
                                  self.componentDescription,
                                  runtimeStateData(_runtime),
                                  [self currentStatePresetName],
                                  [self currentStatePresetNumber]);
}

- (void)setFullState:(NSDictionary<NSString*, id>* _Nullable)fullState {
  NSData* data = nil;
  if ([fullState[CamelCrusherRecalledAUStateBlobKey] isKindOfClass:[NSData class]]) {
    data = fullState[CamelCrusherRecalledAUStateBlobKey];
  } else if ([fullState[CamelCrusherRecalledAUClassInfoDataKey]
                 isKindOfClass:[NSData class]]) {
    data = fullState[CamelCrusherRecalledAUClassInfoDataKey];
  }
  if (data == nil) {
    return;
  }

  if (![self loadRuntimeStateData:data error:nil]) {
    return;
  }

  [self willChangeValueForKey:@"allParameterValues"];
  [self willChangeValueForKey:@"factoryPresets"];
  [self willChangeValueForKey:@"currentPreset"];
  _currentUserPreset = userPresetFromStateDictionary(fullState);
  [self refreshFactoryPresets];
  [self syncParameterValuesFromRuntime];
  [self didChangeValueForKey:@"currentPreset"];
  [self didChangeValueForKey:@"factoryPresets"];
  [self didChangeValueForKey:@"allParameterValues"];
}

- (NSDictionary<NSString*, id>*)fullStateForDocument {
  return self.fullState;
}

- (void)setFullStateForDocument:(NSDictionary<NSString*, id>* _Nullable)fullStateForDocument {
  self.fullState = fullStateForDocument;
}

- (NSArray<AUAudioUnitPreset*>*)factoryPresets {
  return _factoryPresets;
}

- (NSArray<AUAudioUnitPreset*>*)userPresets {
  NSArray<NSURL*>* preset_urls = [[NSFileManager defaultManager]
      contentsOfDirectoryAtURL:userPresetDirectoryURL()
    includingPropertiesForKeys:nil
                       options:NSDirectoryEnumerationSkipsHiddenFiles
                         error:nil];
  if (preset_urls == nil) {
    return @[];
  }

  NSMutableArray<AUAudioUnitPreset*>* presets = [NSMutableArray array];
  for (NSURL* preset_url in preset_urls) {
    if (![[preset_url.pathExtension lowercaseString] isEqualToString:@"aupreset"]) {
      continue;
    }

    NSDictionary<NSString*, id>* preset_state =
        loadPresetDictionaryFromURL(preset_url, nil);
    AUAudioUnitPreset* preset = userPresetFromStateDictionary(preset_state);
    if (preset == nil) {
      continue;
    }
    [presets addObject:preset];
  }

  [presets sortUsingComparator:^NSComparisonResult(AUAudioUnitPreset* left,
                                                   AUAudioUnitPreset* right) {
    return [left.name localizedCaseInsensitiveCompare:right.name];
  }];
  return [presets copy];
}

- (BOOL)supportsUserPresets {
  return YES;
}

- (AUAudioUnitPreset* _Nullable)currentPreset {
  if (_currentUserPreset != nil && _currentUserPreset.number < 0) {
    return _currentUserPreset;
  }

  size_t program_index = 0;
  if (!camelcrusher_runtime_bridge_runtime_get_selected_program_index(_runtime,
                                                                      &program_index)) {
    return nil;
  }

  char name_buffer[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME]{};
  camelcrusher_runtime_bridge_runtime_get_selected_program_name(
      _runtime, name_buffer, sizeof(name_buffer));
  return makePreset(static_cast<NSInteger>(program_index),
                    [NSString stringWithUTF8String:name_buffer]);
}

- (void)setCurrentPreset:(AUAudioUnitPreset* _Nullable)currentPreset {
  if (currentPreset == nil) {
    return;
  }

  if (currentPreset.number < 0) {
    NSError* error = nil;
    NSDictionary<NSString*, id>* preset_state =
        [self presetStateFor:currentPreset error:&error];
    if (preset_state == nil) {
      return;
    }
    self.fullState = preset_state;
    return;
  }

  [self willChangeValueForKey:@"allParameterValues"];
  [self willChangeValueForKey:@"currentPreset"];
  _currentUserPreset = nil;
  camelcrusher_runtime_bridge_runtime_select_legacy_preset(
      _runtime, static_cast<size_t>(currentPreset.number), 1);
  [self syncParameterValuesFromRuntime];
  [self didChangeValueForKey:@"currentPreset"];
  [self didChangeValueForKey:@"allParameterValues"];
}

- (BOOL)saveUserPreset:(AUAudioUnitPreset*)userPreset error:(NSError**)outError {
  if (userPreset == nil) {
    return NO;
  }

  userPreset.number = normalizedUserPresetNumber(userPreset.number);
  userPreset.name = userPreset.name.length > 0U ? userPreset.name : @"Imported Preset";

  NSDictionary<NSString*, id>* preset_state =
      normalizedClassInfoState(nil, self.componentDescription,
                               runtimeStateData(_runtime), userPreset.name,
                               userPreset.number);
  if (!writePresetDictionaryToURL(preset_state, userPresetFileURL(userPreset.name),
                                  outError)) {
    return NO;
  }

  [self willChangeValueForKey:@"currentPreset"];
  [self willChangeValueForKey:@"userPresets"];
  _currentUserPreset = makePreset(userPreset.number, userPreset.name);
  [self didChangeValueForKey:@"userPresets"];
  [self didChangeValueForKey:@"currentPreset"];
  return YES;
}

- (BOOL)deleteUserPreset:(AUAudioUnitPreset*)userPreset error:(NSError**)outError {
  if (userPreset == nil || userPreset.name.length == 0U) {
    if (outError != nullptr) {
      *outError = [NSError errorWithDomain:NSOSStatusErrorDomain
                                      code:kAudioUnitErr_InvalidFilePath
                                  userInfo:@{
                                    NSLocalizedDescriptionKey :
                                        @"User preset name is required"
                                  }];
    }
    return NO;
  }

  NSURL* preset_url = userPresetFileURL(userPreset.name);
  if (![[NSFileManager defaultManager] removeItemAtURL:preset_url error:outError]) {
    return NO;
  }

  [self willChangeValueForKey:@"userPresets"];
  if (_currentUserPreset != nil && [_currentUserPreset.name isEqualToString:userPreset.name]) {
    [self willChangeValueForKey:@"currentPreset"];
    _currentUserPreset = nil;
    [self didChangeValueForKey:@"currentPreset"];
  }
  [self didChangeValueForKey:@"userPresets"];
  return YES;
}

- (NSDictionary<NSString*, id>* _Nullable)presetStateFor:(AUAudioUnitPreset*)userPreset
                                                    error:(NSError**)outError {
  if (userPreset.number < 0 && userPreset.name.length > 0U) {
    NSURL* preset_url = userPresetFileURL(userPreset.name);
    NSDictionary<NSString*, id>* saved_state =
        loadPresetDictionaryFromURL(preset_url, outError);
    if (saved_state != nil) {
      return normalizedClassInfoState(saved_state, self.componentDescription,
                                      runtimeStateData(_runtime), userPreset.name,
                                      normalizedUserPresetNumber(userPreset.number));
    }

    if (_currentUserPreset != nil &&
        _currentUserPreset.number == normalizedUserPresetNumber(userPreset.number) &&
        [_currentUserPreset.name isEqualToString:userPreset.name]) {
      if (outError != nullptr) {
        *outError = nil;
      }
      return normalizedClassInfoState(nil, self.componentDescription,
                                      runtimeStateData(_runtime), userPreset.name,
                                      normalizedUserPresetNumber(userPreset.number));
    }

    return nil;
  }

  NSDictionary<NSString*, id>* preset_state =
      [super presetStateFor:userPreset error:outError];
  if (preset_state == nil) {
    if (_currentUserPreset != nil && _currentUserPreset.number == userPreset.number) {
      if (outError != nullptr) {
        *outError = nil;
      }
      return normalizedClassInfoState(nil, self.componentDescription,
                                      runtimeStateData(_runtime),
                                      userPreset.name ?: [self currentStatePresetName],
                                      userPreset.number);
    }
    return nil;
  }

  return normalizedClassInfoState(preset_state, self.componentDescription,
                                  runtimeStateData(_runtime),
                                  userPreset.name ?: @"Imported Preset",
                                  userPreset.number);
}

- (void)refreshFactoryPresets {
  const auto preset_count =
      camelcrusher_runtime_bridge_runtime_preset_count(_runtime);
  NSMutableArray<AUAudioUnitPreset*>* presets =
      [NSMutableArray arrayWithCapacity:preset_count];
  for (size_t index = 0; index < preset_count; ++index) {
    char name_buffer[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME]{};
    if (!camelcrusher_runtime_bridge_runtime_get_legacy_preset_name(
            _runtime, index, name_buffer, sizeof(name_buffer))) {
      continue;
    }
    AUAudioUnitPreset* preset = [AUAudioUnitPreset new];
    preset.number = static_cast<NSInteger>(index);
    preset.name = [NSString stringWithUTF8String:name_buffer];
    [presets addObject:preset];
  }
  _factoryPresets = [presets copy];
}

- (NSInteger)currentStatePresetNumber {
  if (_currentUserPreset != nil && _currentUserPreset.number < 0) {
    return _currentUserPreset.number;
  }

  size_t program_index = 0;
  if (camelcrusher_runtime_bridge_runtime_get_selected_program_index(_runtime,
                                                                     &program_index)) {
    return static_cast<NSInteger>(program_index);
  }
  return -1;
}

- (NSString*)currentStatePresetName {
  if (_currentUserPreset != nil && _currentUserPreset.name.length > 0U) {
    return _currentUserPreset.name;
  }

  char name_buffer[CAMELCRUSHER_RUNTIME_BRIDGE_MAX_PROGRAM_NAME]{};
  camelcrusher_runtime_bridge_runtime_get_selected_program_name(
      _runtime, name_buffer, sizeof(name_buffer));
  if (name_buffer[0] == '\0') {
    return @"Current State";
  }
  return [NSString stringWithUTF8String:name_buffer];
}

- (AUParameterTree*)buildParameterTree {
  NSMutableArray<AUParameterNode*>* parameters = [NSMutableArray array];
  const auto parameter_count = camelcrusher_runtime_bridge_public_parameter_count();
  for (size_t index = 0; index < parameter_count; ++index) {
    CamelCrusherRuntimeBridgePublicParameterDescriptor descriptor{};
    if (!camelcrusher_runtime_bridge_get_public_parameter_descriptor(index,
                                                                     &descriptor)) {
      continue;
    }

    NSString* identifier =
        [NSString stringWithUTF8String:descriptor.stable_id ?: ""];
    NSString* name =
        [NSString stringWithUTF8String:descriptor.display_name ?: ""];
    AUParameter* parameter = [AUParameterTree
        createParameterWithIdentifier:identifier
                                 name:name
                              address:descriptor.public_index
                                  min:0.0F
                                  max:1.0F
                                 unit:parameterUnitForDescriptor(descriptor)
                             unitName:nil
                                flags:parameterFlagsForDescriptor(descriptor)
                         valueStrings:nil
                  dependentParameters:nil];
    [parameters addObject:parameter];
  }

  AUParameterTree* tree = [AUParameterTree createTreeWithChildren:parameters];
  __block CamelCrusherRuntimeBridgeRuntime* runtime = _runtime;
  tree.implementorValueObserver = ^(AUParameter* param, AUValue value) {
    camelcrusher_runtime_bridge_runtime_set_public_parameter(
        runtime, static_cast<size_t>(param.address), value);
  };
  tree.implementorValueProvider = ^AUValue(AUParameter* param) {
    return camelcrusher_runtime_bridge_runtime_get_public_parameter(
        runtime, static_cast<size_t>(param.address));
  };
  tree.implementorStringFromValueCallback =
      ^NSString*(AUParameter* param, const AUValue* value) {
        const auto display_value =
            value != nullptr
                ? *value
                : camelcrusher_runtime_bridge_runtime_get_public_parameter(
                      runtime, static_cast<size_t>(param.address));
        if (param.unit == kAudioUnitParameterUnit_Boolean) {
          return display_value >= 0.5F ? @"On" : @"Off";
        }
        return [NSString stringWithFormat:@"%.3f", display_value];
      };
  return tree;
}

- (void)syncParameterValuesFromRuntime {
  for (AUParameter* parameter in _parameterTree.allParameters) {
    parameter.value = camelcrusher_runtime_bridge_runtime_get_public_parameter(
        _runtime, static_cast<size_t>(parameter.address));
  }
}

- (BOOL)loadRuntimeStateData:(NSData*)data error:(NSError* _Nullable __autoreleasing*)outError {
  CamelCrusherRuntimeBridgeTextError error{};
  if (!camelcrusher_runtime_bridge_runtime_load_state(
          _runtime, static_cast<const uint8_t*>(data.bytes), data.length, &error)) {
    if (outError != nullptr) {
      *outError = makeRuntimeBridgeError(error);
    }
    return NO;
  }
  return YES;
}

@end
