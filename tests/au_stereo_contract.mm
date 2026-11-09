#import "camelcrusher_recalled/CamelCrusherRecalledAudioUnit.h"
#import <AVFoundation/AVFoundation.h>
#include "StereoCases.h"

struct BufferList { UInt32 count; AudioBuffer buffers[2]; };

int main() {
  @autoreleasepool {
    int cases = 0;
    for (double rate : {44100.0, 48000.0, 96000.0}) {
      for (bool packed_input : {false, true}) {
        for (bool packed_output : {false, true}) {
          NSError* error = nil;
          auto* unit = [[CamelCrusherRecalledAudioUnit alloc]
              initWithComponentDescription:CamelCrusherRecalledAudioUnit.componentDescription
              options:0 error:&error];
          CC_CHECK(unit != nil);
          auto* input_format = [[AVAudioFormat alloc]
              initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:rate
              channels:2 interleaved:packed_input];
          auto* output_format = [[AVAudioFormat alloc]
              initWithCommonFormat:AVAudioPCMFormatFloat32 sampleRate:rate
              channels:2 interleaved:packed_output];
          const BOOL accepted_input = [unit.inputBusses[0] setFormat:input_format error:&error];
          CC_CHECK(accepted_input == !packed_input);
          if (packed_input) CC_CHECK(error.code == kAudioUnitErr_FormatNotSupported);
          error = nil;
          const BOOL accepted_output = [unit.outputBusses[0] setFormat:output_format error:&error];
          CC_CHECK(accepted_output == !packed_output);
          if (packed_output) CC_CHECK(error.code == kAudioUnitErr_FormatNotSupported);
          if (packed_input || packed_output) continue;
          unit.maximumFramesToRender = 1024;
          CC_CHECK([unit allocateRenderResourcesAndReturnError:&error]);
          [unit.parameterTree parameterWithAddress:9].value = 0.0F;
          auto render = unit.renderBlock;
          for (int size : {1, 128, 257, 512, 1024}) {
            for (int pattern = 0; pattern < 6; ++pattern) {
              for (bool null_output : {false, true}) {
                StereoCase signal(size, pattern);
                std::vector<float> left(size), right(size), packed(size * 2);
                BufferList output{};
                output.count = packed_output ? 1 : 2;
                output.buffers[0] = {
                    packed_output ? 2U : 1U,
                    static_cast<UInt32>(size * sizeof(float) * (packed_output ? 2 : 1)),
                    null_output ? nullptr : (packed_output ? packed.data() : left.data())};
                output.buffers[1] = {1, static_cast<UInt32>(size * sizeof(float)),
                                     null_output ? nullptr : right.data()};
                const float* lp = signal.left.data();
                const float* rp = signal.right.data();
                AURenderPullInputBlock pull = ^AUAudioUnitStatus(
                    AudioUnitRenderActionFlags*, const AudioTimeStamp*,
                    AUAudioFrameCount count, NSInteger, AudioBufferList* dest) {
                  CC_CHECK(dest->mNumberBuffers == (packed_input ? 1U : 2U));
                  auto* l = static_cast<float*>(dest->mBuffers[0].mData);
                  auto* r = packed_input ? l + 1 : static_cast<float*>(dest->mBuffers[1].mData);
                  const unsigned stride = packed_input ? 2 : 1;
                  for (unsigned i = 0; i < count; ++i) {
                    l[i * stride] = lp[i];
                    r[i * stride] = rp[i];
                  }
                  return noErr;
                };
                AudioTimeStamp time{};
                time.mFlags = kAudioTimeStampSampleTimeValid;
                AudioUnitRenderActionFlags flags = 0;
                CC_CHECK(render(&flags, &time, size, 0,
                    reinterpret_cast<AudioBufferList*>(&output), pull) == noErr);
                auto* l = static_cast<float*>(output.buffers[0].mData);
                auto* r = packed_output ? l + 1 : static_cast<float*>(output.buffers[1].mData);
                for (int i = 0; i < size; ++i) {
                  checkSample(l[i * (packed_output ? 2 : 1)], signal.left[i]);
                  checkSample(r[i * (packed_output ? 2 : 1)], signal.right[i]);
                }
                ++cases;
              }
            }
          }
          [unit deallocateRenderResources];
        }
      }
    }
    std::cout << cases << " AU routing cases with negotiated layouts passed\n";
  }
}
