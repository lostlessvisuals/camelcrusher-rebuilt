#pragma once

#include "aeffect.h"

#include <cstddef>
#include <cstdint>

template <typename T>
inline T* FromVstPtr(const VstIntPtr ptr) {
  return reinterpret_cast<T*>(ptr);
}

template <typename T>
inline VstIntPtr ToVstPtr(T* ptr) {
  return reinterpret_cast<VstIntPtr>(ptr);
}

enum VstProcessLevels {
  kVstProcessLevelUnknown = 0,
  kVstProcessLevelUser,
  kVstProcessLevelRealtime,
  kVstProcessLevelPrefetch,
  kVstProcessLevelOffline,
};

enum VstAutomationStates {
  kVstAutomationUnsupported = 0,
  kVstAutomationOff,
  kVstAutomationRead,
  kVstAutomationWrite,
  kVstAutomationReadWrite,
};

enum VstHostLanguage {
  kVstLangEnglish = 1,
  kVstLangGerman,
  kVstLangFrench,
  kVstLangItalian,
  kVstLangSpanish,
  kVstLangJapanese,
};

enum VstProcessPrecision {
  kVstProcessPrecision32 = 0,
  kVstProcessPrecision64 = 1,
};

enum VstParameterFlags {
  kVstParameterIsSwitch = 1 << 0,
  kVstParameterUsesIntegerMinMax = 1 << 1,
  kVstParameterUsesFloatStep = 1 << 2,
  kVstParameterUsesIntStep = 1 << 3,
  kVstParameterSupportsDisplayIndex = 1 << 4,
  kVstParameterSupportsDisplayCategory = 1 << 5,
  kVstParameterCanRamp = 1 << 6,
};

struct VstParameterProperties {
  float stepFloat = 0.0F;
  float smallStepFloat = 0.0F;
  float largeStepFloat = 0.0F;
  char label[kVstMaxLabelLen]{};
  VstInt32 flags = 0;
  VstInt32 minInteger = 0;
  VstInt32 maxInteger = 0;
  VstInt32 stepInteger = 0;
  VstInt32 largeStepInteger = 0;
  char shortLabel[kVstMaxShortLabelLen]{};
  VstInt16 displayIndex = 0;
  VstInt16 category = 0;
  VstInt16 numParametersInCategory = 0;
  VstInt16 reserved = 0;
  char categoryLabel[kVstMaxCategLabelLen]{};
  char future[16]{};
};

enum VstSpeakerArrangementType {
  kSpeakerArrUserDefined = -2,
  kSpeakerArrEmpty = -1,
  kSpeakerArrMono = 0,
  kSpeakerArrStereo = 1,
  kSpeakerArr51 = 8,
};

struct VstSpeakerProperties {
  float azimuth = 0.0F;
  float elevation = 0.0F;
  float radius = 0.0F;
  float reserved = 0.0F;
  char name[kVstMaxNameLen]{};
  VstInt32 type = 0;
  char future[28]{};
};

struct VstSpeakerArrangement {
  VstInt32 type = kSpeakerArrEmpty;
  VstInt32 numChannels = 0;
  VstSpeakerProperties speakers[8]{};
};

enum VstTimeInfoFlags {
  kVstTransportChanged = 1 << 1,
  kVstTransportPlaying = 1 << 2,
  kVstTransportCycleActive = 1 << 3,
  kVstTransportRecording = 1 << 4,
  kVstAutomationWriting = 1 << 6,
  kVstAutomationReading = 1 << 7,
  kVstNanosValid = 1 << 8,
  kVstPpqPosValid = 1 << 9,
  kVstTempoValid = 1 << 10,
  kVstBarsValid = 1 << 11,
  kVstCyclePosValid = 1 << 12,
  kVstTimeSigValid = 1 << 13,
  kVstSmpteValid = 1 << 14,
  kVstClockValid = 1 << 15,
};

struct VstTimeInfo {
  double samplePos = 0.0;
  double sampleRate = 0.0;
  double nanoSeconds = 0.0;
  double ppqPos = 0.0;
  double tempo = 120.0;
  double barStartPos = 0.0;
  double cycleStartPos = 0.0;
  double cycleEndPos = 0.0;
  VstInt32 timeSigNumerator = 4;
  VstInt32 timeSigDenominator = 4;
  VstInt32 smpteOffset = 0;
  VstInt32 smpteFrameRate = 0;
  VstInt32 samplesToNextClock = 0;
  VstInt32 flags = 0;
};

enum VstEventTypes {
  kVstMidiType = 1,
};

struct VstEvent {
  VstInt32 type = 0;
  VstInt32 byteSize = 0;
  VstInt32 deltaFrames = 0;
  VstInt32 flags = 0;
  char data[16]{};
};

enum VstMidiEventFlags {
  kVstMidiEventFlagIsRealtime = 1 << 0,
};

struct VstMidiEvent {
  VstInt32 type = kVstMidiType;
  VstInt32 byteSize = sizeof(VstMidiEvent);
  VstInt32 deltaFrames = 0;
  VstInt32 flags = 0;
  VstInt32 noteLength = 0;
  VstInt32 noteOffset = 0;
  std::uint8_t midiData[4]{};
  char detune = 0;
  char noteOffVelocity = 0;
  char reserved1 = 0;
  char reserved2 = 0;
};

struct VstEvents {
  VstInt32 numEvents = 0;
  VstIntPtr reserved = 0;
  VstEvent* events[2]{};
};

struct VstVariableIo {
  float** inputs = nullptr;
  float** outputs = nullptr;
  VstInt32 numSamplesInput = 0;
  VstInt32 numSamplesOutput = 0;
  VstInt32* numSamplesInputProcessed = nullptr;
  VstInt32* numSamplesOutputProcessed = nullptr;
};

enum VstOfflineOption {
  kVstOfflineAudio = 0,
};

struct VstOfflineTask {
  char processName[kVstMaxNameLen]{};
  double readPosition = 0.0;
  double writePosition = 0.0;
  VstInt32 numFramesToProcess = 0;
  VstInt32 maxFramesToWrite = 0;
  void* sourceBuffer = nullptr;
  void* destinationBuffer = nullptr;
};

struct VstAudioFile {
  VstInt32 flags = 0;
  VstInt32 hostOwned = 0;
  VstInt32 sampleRate = 0;
  VstInt32 numChannels = 0;
  VstInt32 numFrames = 0;
  VstInt32 format = 0;
  char path[1024]{};
};

struct VstWindow {
  VstInt32 xPos = 0;
  VstInt32 yPos = 0;
  VstInt32 width = 0;
  VstInt32 height = 0;
  void* parent = nullptr;
  void* userHandle = nullptr;
};

struct VstKeyCode {
  VstInt32 character = 0;
  std::uint8_t virt = 0;
  std::uint8_t modifier = 0;
};

struct VstPatchChunkInfo {
  VstInt32 version = 1;
  VstInt32 pluginUniqueID = 0;
  VstInt32 pluginVersion = 0;
  VstInt32 numElements = 0;
  char future[48]{};
};

struct VstFileSelect {
  VstInt32 command = 0;
  VstInt32 type = 0;
  VstInt32 macCreator = 0;
  VstInt32 nbFileTypes = 0;
  const void* fileTypes = nullptr;
  char title[1024]{};
  char initialPath[1024]{};
  VstIntPtr future = 0;
};

struct MidiProgramName {
  VstInt32 thisProgramIndex = 0;
  char name[64]{};
  VstInt32 midiProgram = 0;
  VstInt32 midiBankMsb = 0;
  VstInt32 midiBankLsb = 0;
  VstInt32 parentCategoryIndex = -1;
  VstInt32 flags = 0;
};

struct MidiProgramCategory {
  VstInt32 thisCategoryIndex = 0;
  char name[64]{};
  VstInt32 parentCategoryIndex = -1;
  VstInt32 flags = 0;
};

struct MidiKeyName {
  VstInt32 thisProgramIndex = 0;
  VstInt32 thisKeyNumber = 0;
  char keyName[64]{};
  VstInt32 reserved = 0;
};

