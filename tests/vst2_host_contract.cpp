// Deliberately independent of the plug-in's SDK and ABI headers.
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <dlfcn.h>
#include <vector>

namespace {
using I32 = std::int32_t;
using IP = std::intptr_t;
struct Effect;
using Callback = IP (*)(Effect*, I32, I32, IP, void*, float);
using Process = void (*)(Effect*, float**, float**, I32);
struct Effect {
  I32 magic;
  Callback dispatcher;
  Process process;
  void (*setParameter)(Effect*, I32, float);
  float (*getParameter)(Effect*, I32);
  I32 programs, parameters, inputs, outputs, flags;
  void* reserved[2];
  I32 delay, realQuality, offlineQuality;
  float ratio;
  void* object;
  void* user;
  I32 id, version;
  Process replacing;
  void (*doubleReplacing)(Effect*, double**, double**, I32);
  char future[56];
};
struct Pin {
  char label[64];
  I32 flags, arrangement;
  char shortLabel[8], future[48];
};
struct Speaker {
  float azimuth, elevation, radius, reserved;
  char name[64];
  I32 type;
  char future[28];
};
struct Arrangement { I32 type, count; Speaker speakers[8]; };
static_assert(sizeof(Effect) == 192 && offsetof(Effect, replacing) == 120);
static_assert(sizeof(Pin) == 128 && offsetof(Pin, flags) == 64);
static_assert(sizeof(Speaker) == 112 && offsetof(Speaker, type) == 80);

IP host(Effect*, I32 opcode, I32, IP, void*, float) {
  if (opcode == 1) return 2400;
  if (opcode == 16) return 48000;
  if (opcode == 17) return 512;
  return 0;
}
IP dispatch(Effect* e, I32 op, I32 index = 0, IP value = 0,
            void* ptr = nullptr, float opt = 0.0F) {
  return e->dispatcher(e, op, index, value, ptr, opt);
}
void layout(Effect* e, bool mono) {
  Arrangement input{}, output{};
  input.type = mono ? 0 : 1;
  input.count = mono ? 1 : 2;
  output.type = 1;
  output.count = 2;
  CC_CHECK(dispatch(e, 42, 0, reinterpret_cast<IP>(&input), &output) == 1);
}
void equal(const std::vector<float>& actual, const std::vector<float>& expected,
           float addition = 0.0F) {
  for (std::size_t i = 0; i < actual.size(); ++i) {
    CC_CHECK(std::isfinite(actual[i]));
    CC_CHECK(std::abs(actual[i] - (expected[i] + addition)) < 1.0e-7F);
  }
}
}  // namespace

int main(int argc, char** argv) {
  const char* path = argc == 2 ? argv[1] : CAMELCRUSHER_VST2_BINARY;
  void* library = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  if (!library) std::cerr << dlerror() << '\n';
  CC_CHECK(library != nullptr);
  auto entry = reinterpret_cast<Effect* (*)(Callback)>(dlsym(library, "VSTPluginMain"));
  CC_CHECK(entry != nullptr);
  Effect* e = entry(host);
  CC_CHECK(e != nullptr && e->magic == 0x56737450);
  CC_CHECK(e->inputs == 2 && e->outputs == 2 && e->id == 0x43614372);
  CC_CHECK(e->programs == 20 && e->parameters == 17 && e->version == 1);
  dispatch(e, 0);
  for (I32 op : {33, 34}) {
    for (I32 channel = 0; channel < 2; ++channel) {
      Pin pin{};
      CC_CHECK(dispatch(e, op, channel, 0, &pin) == 1);
      CC_CHECK(pin.flags == (channel == 0 ? 7 : 5));
      CC_CHECK(pin.arrangement == 1);
    }
  }
  Arrangement *input = nullptr, *output = nullptr;
  CC_CHECK(dispatch(e, 69, 0, reinterpret_cast<IP>(&input), &output) == 1);
  CC_CHECK(input && output && input->count == 2 && output->count == 2);
  CC_CHECK(input->speakers[0].type == 1 && input->speakers[1].type == 2);
  CC_CHECK(output->speakers[0].type == 1 && output->speakers[1].type == 2);

  int cases = 0;
  for (float rate : {44100.0F, 48000.0F, 96000.0F}) {
    dispatch(e, 10, 0, 0, nullptr, rate);
    dispatch(e, 11, 0, 1024);
    dispatch(e, 12, 0, 1);
    e->setParameter(e, 9, 0.0F);
    for (int size : {1, 7, 128, 256, 257, 512, 1024, 4097}) {
      for (int pattern = 0; pattern < 6; ++pattern) {
        std::vector<float> l(size, 0.25F), r(size, 0.0F);
        if (pattern == 1) { std::fill(l.begin(), l.end(), 0.0F); r.back() = -0.5F; }
        if (pattern == 2) std::fill(r.begin(), r.end(), -0.125F);
        if (pattern == 3) r.back() = -0.5F;
        if (pattern == 4) std::fill(r.begin(), r.end(), 1.0e-8F);
        if (pattern == 5) std::fill(l.begin(), l.end(), 0.0F);
        for (bool mono : {false, true}) {
          layout(e, mono);
          float* in[] = {l.data(), mono ? nullptr : r.data()};
          std::vector<float> ol(size), oright(size);
          float* out[] = {ol.data(), oright.data()};
          e->replacing(e, in, out, size);
          equal(ol, l); equal(oright, mono ? l : r);
          std::fill(ol.begin(), ol.end(), 0.1F);
          std::fill(oright.begin(), oright.end(), 0.1F);
          e->process(e, in, out, size);
          equal(ol, l, 0.1F); equal(oright, mono ? l : r, 0.1F);
          ol = l; oright = r;
          float* inplace_in[] = {ol.data(), mono ? nullptr : oright.data()};
          e->replacing(e, inplace_in, out, size);
          equal(ol, l); equal(oright, mono ? l : r);
          ++cases;
        }
      }
    }
    dispatch(e, 12, 0, 0);
  }

  // All factory presets must preserve channel symmetry with processing enabled.
  layout(e, false);
  Effect* mirror = entry(host);
  dispatch(mirror, 0);
  for (int preset = 0; preset < 20; ++preset) {
    for (Effect* instance : {e, mirror}) {
      dispatch(instance, 2, 0, preset);
      dispatch(instance, 10, 0, 0, nullptr, 48000.0F);
      dispatch(instance, 11, 0, 512);
      dispatch(instance, 12, 0, 1);
      instance->setParameter(instance, 9, 1.0F);
    }
    std::vector<float> l(512), r(512), ol(512), oright(512), ml(512), mr(512);
    for (int i = 0; i < 512; ++i) {
      l[i] = 0.3F * std::sin(i * 0.13F);
      r[i] = i < 300 ? 0.0F : -0.2F * std::cos(i * 0.07F);
    }
    float* in[] = {l.data(), r.data()};
    float* swapped[] = {r.data(), l.data()};
    float* out[] = {ol.data(), oright.data()};
    float* mout[] = {ml.data(), mr.data()};
    e->replacing(e, in, out, 512);
    mirror->replacing(mirror, swapped, mout, 512);
    equal(ol, mr); equal(oright, ml);
    bool distinct = false;
    for (int i = 0; i < 512; ++i) distinct |= std::abs(ol[i] - oright[i]) > 1.0e-5F;
    CC_CHECK(distinct);
  }
  dispatch(mirror, 1);
  dispatch(e, 1);
  dlclose(library);
  std::cout << cases << " routing cases plus 20 wet preset symmetry checks passed\n";
}
