#pragma once

#include <cstdint>
#include <cstring>

#ifndef DECLARE_VST_DEPRECATED
#define DECLARE_VST_DEPRECATED(symbol) symbol
#endif

#ifndef VST_FORCE_DEPRECATED
#define VST_FORCE_DEPRECATED 0
#endif

#ifndef VST_2_1_EXTENSIONS
#define VST_2_1_EXTENSIONS 1
#endif

#ifndef VST_2_3_EXTENSIONS
#define VST_2_3_EXTENSIONS 1
#endif

#ifndef VST_2_4_EXTENSIONS
#define VST_2_4_EXTENSIONS 1
#endif

#ifndef TARGET_API_MAC_CARBON
#define TARGET_API_MAC_CARBON 0
#endif

using VstInt8 = std::int8_t;
using VstInt16 = std::int16_t;
using VstInt32 = std::int32_t;
using VstInt64 = std::int64_t;
using VstIntPtr = std::intptr_t;

struct AEffect;

using audioMasterCallback =
    VstIntPtr (*)(AEffect* effect,
                  VstInt32 opcode,
                  VstInt32 index,
                  VstIntPtr value,
                  void* ptr,
                  float opt);
using AEffectDispatcherProc =
    VstIntPtr (*)(AEffect* effect,
                  VstInt32 opcode,
                  VstInt32 index,
                  VstIntPtr value,
                  void* ptr,
                  float opt);
using AEffectProcessProc =
    void (*)(AEffect* effect, float** inputs, float** outputs, VstInt32 sampleFrames);
using AEffectSetParameterProc =
    void (*)(AEffect* effect, VstInt32 index, float parameter);
using AEffectGetParameterProc = float (*)(AEffect* effect, VstInt32 index);
using AEffectProcessDoubleProc =
    void (*)(AEffect* effect,
             double** inputs,
             double** outputs,
             VstInt32 sampleFrames);

constexpr VstInt32 kVstVersion = 2400;
#ifndef CCONST
#define CCONST(a, b, c, d)                                                    \
  ((static_cast<VstInt32>(a) << 24) | (static_cast<VstInt32>(b) << 16) |      \
   (static_cast<VstInt32>(c) << 8) | static_cast<VstInt32>(d))
#endif
constexpr VstInt32 kEffectMagic =
    CCONST('V', 's', 't', 'P');

constexpr VstInt32 kVstMaxProgNameLen = 24;
constexpr VstInt32 kVstMaxParamStrLen = 8;
constexpr VstInt32 kVstMaxVendorStrLen = 64;
constexpr VstInt32 kVstMaxProductStrLen = 64;
constexpr VstInt32 kVstMaxEffectNameLen = 32;
constexpr VstInt32 kVstMaxLabelLen = 64;
constexpr VstInt32 kVstMaxShortLabelLen = 8;
constexpr VstInt32 kVstMaxCategLabelLen = 24;
constexpr VstInt32 kVstMaxNameLen = 64;

inline void vst_strncpy(char* destination, const char* source, VstInt32 maxLength) {
  if (destination == nullptr || maxLength <= 0) {
    return;
  }
  if (source == nullptr) {
    destination[0] = '\0';
    return;
  }

  const auto max_copy = static_cast<std::size_t>(maxLength - 1);
  std::strncpy(destination, source, max_copy);
  destination[max_copy] = '\0';
}

inline void vst_strncat(char* destination, const char* source, VstInt32 maxLength) {
  if (destination == nullptr || source == nullptr || maxLength <= 0) {
    return;
  }

  const auto used = std::strlen(destination);
  if (used >= static_cast<std::size_t>(maxLength - 1)) {
    return;
  }

  vst_strncpy(destination + used, source, maxLength - static_cast<VstInt32>(used));
}

struct ERect {
  VstInt16 top;
  VstInt16 left;
  VstInt16 bottom;
  VstInt16 right;
};

enum VstDispatcherOpCode {
  effOpen = 0,
  effClose,
  effSetProgram,
  effGetProgram,
  effSetProgramName,
  effGetProgramName,
  effGetParamLabel,
  effGetParamDisplay,
  effGetParamName,
  DECLARE_VST_DEPRECATED(effGetVu),
  effSetSampleRate,
  effSetBlockSize,
  effMainsChanged,
  effEditGetRect,
  effEditOpen,
  effEditClose,
  DECLARE_VST_DEPRECATED(effEditDraw),
  DECLARE_VST_DEPRECATED(effEditMouse),
  DECLARE_VST_DEPRECATED(effEditKey),
  DECLARE_VST_DEPRECATED(effEditIdle),
  DECLARE_VST_DEPRECATED(effEditTop),
  DECLARE_VST_DEPRECATED(effEditSleep),
  DECLARE_VST_DEPRECATED(effIdentify),
  effGetChunk,
  effSetChunk,
  effProcessEvents,
  effCanBeAutomated,
  effString2Parameter,
  DECLARE_VST_DEPRECATED(effGetNumProgramCategories),
  effGetProgramNameIndexed,
  DECLARE_VST_DEPRECATED(effCopyProgram),
  DECLARE_VST_DEPRECATED(effConnectInput),
  DECLARE_VST_DEPRECATED(effConnectOutput),
  effGetInputProperties,
  effGetOutputProperties,
  effGetPlugCategory,
  DECLARE_VST_DEPRECATED(effGetCurrentPosition),
  DECLARE_VST_DEPRECATED(effGetDestinationBuffer),
  effOfflineNotify,
  effOfflinePrepare,
  effOfflineRun,
  effProcessVarIo,
  effSetSpeakerArrangement,
  DECLARE_VST_DEPRECATED(effSetBlockSizeAndSampleRate),
  effSetBypass,
  effGetEffectName,
  DECLARE_VST_DEPRECATED(effGetErrorText),
  effGetVendorString,
  effGetProductString,
  effGetVendorVersion,
  effVendorSpecific,
  effCanDo,
  effGetTailSize,
  DECLARE_VST_DEPRECATED(effIdle),
  DECLARE_VST_DEPRECATED(effGetIcon),
  DECLARE_VST_DEPRECATED(effSetViewPosition),
  effGetParameterProperties,
  DECLARE_VST_DEPRECATED(effKeysRequired),
  effGetVstVersion,
  effEditKeyDown,
  effEditKeyUp,
  effSetEditKnobMode,
  effGetMidiProgramName,
  effGetCurrentMidiProgram,
  effGetMidiProgramCategory,
  effHasMidiProgramsChanged,
  effGetMidiKeyName,
  effBeginSetProgram,
  effEndSetProgram,
  effGetSpeakerArrangement,
  effShellGetNextPlugin,
  effStartProcess,
  effStopProcess,
  effSetTotalSampleToProcess,
  effSetPanLaw,
  effBeginLoadBank,
  effBeginLoadProgram,
  effSetProcessPrecision,
  effGetNumMidiInputChannels,
  effGetNumMidiOutputChannels,
};

enum VstAEffectFlags {
  effFlagsHasEditor = 1 << 0,
  DECLARE_VST_DEPRECATED(effFlagsHasClip) = 1 << 1,
  DECLARE_VST_DEPRECATED(effFlagsHasVu) = 1 << 2,
  effFlagsCanMono = 1 << 3,
  effFlagsCanReplacing = 1 << 4,
  effFlagsProgramChunks = 1 << 5,
  effFlagsIsSynth = 1 << 8,
  effFlagsNoSoundInStop = 1 << 9,
  DECLARE_VST_DEPRECATED(effFlagsExtIsAsync) = 1 << 10,
  DECLARE_VST_DEPRECATED(effFlagsExtHasBuffer) = 1 << 11,
  effFlagsCanDoubleReplacing = 1 << 12,
};

struct AEffect {
  VstInt32 magic = kEffectMagic;
  AEffectDispatcherProc dispatcher = nullptr;
  DECLARE_VST_DEPRECATED(AEffectProcessProc) process = nullptr;
  AEffectSetParameterProc setParameter = nullptr;
  AEffectGetParameterProc getParameter = nullptr;
  VstInt32 numPrograms = 0;
  VstInt32 numParams = 0;
  VstInt32 numInputs = 0;
  VstInt32 numOutputs = 0;
  VstInt32 flags = 0;
  void* resvd1 = nullptr;
  void* resvd2 = nullptr;
  VstInt32 initialDelay = 0;
  VstInt32 realQualities = 0;
  VstInt32 offQualities = 0;
  float DECLARE_VST_DEPRECATED(ioRatio) = 1.0F;
  void* object = nullptr;
  void* user = nullptr;
  VstInt32 uniqueID = 0;
  VstInt32 version = 0;
  AEffectProcessProc processReplacing = nullptr;
  AEffectProcessDoubleProc processDoubleReplacing = nullptr;
  char future[56]{};
};

enum VstPlugCategory {
  kPlugCategUnknown = 0,
  kPlugCategEffect,
  kPlugCategSynth,
  kPlugCategAnalysis,
  kPlugCategMastering,
  kPlugCategSpacializer,
  kPlugCategRoomFx,
  kPlugCategSurroundFx,
  kPlugCategRestoration,
  kPlugCategOfflineProcess,
  kPlugCategShell,
  kPlugCategGenerator,
};

enum VstPinPropertiesFlags {
  kVstPinIsActive = 1 << 0,
  kVstPinIsStereo = 1 << 1,
  kVstPinUseSpeaker = 1 << 2,
};

struct VstPinProperties {
  char label[kVstMaxLabelLen]{};
  char shortLabel[kVstMaxShortLabelLen]{};
  VstInt32 arrangementType = 0;
  VstInt32 flags = 0;
  char future[48]{};
};

enum AudioMasterOpcodes {
  audioMasterAutomate = 0,
  audioMasterVersion,
  audioMasterCurrentId,
  audioMasterIdle,
  DECLARE_VST_DEPRECATED(audioMasterPinConnected),
  DECLARE_VST_DEPRECATED(audioMasterWantMidi),
  audioMasterGetTime,
  audioMasterProcessEvents,
  DECLARE_VST_DEPRECATED(audioMasterSetTime),
  DECLARE_VST_DEPRECATED(audioMasterTempoAt),
  DECLARE_VST_DEPRECATED(audioMasterGetNumAutomatableParameters),
  DECLARE_VST_DEPRECATED(audioMasterGetParameterQuantization),
  audioMasterIOChanged,
  DECLARE_VST_DEPRECATED(audioMasterNeedIdle),
  audioMasterSizeWindow,
  audioMasterGetSampleRate,
  audioMasterGetBlockSize,
  audioMasterGetInputLatency,
  audioMasterGetOutputLatency,
  DECLARE_VST_DEPRECATED(audioMasterGetPreviousPlug),
  DECLARE_VST_DEPRECATED(audioMasterGetNextPlug),
  DECLARE_VST_DEPRECATED(audioMasterWillReplaceOrAccumulate),
  audioMasterGetCurrentProcessLevel,
  audioMasterGetAutomationState,
  audioMasterOfflineStart,
  audioMasterOfflineRead,
  audioMasterOfflineWrite,
  audioMasterOfflineGetCurrentPass,
  audioMasterOfflineGetCurrentMetaPass,
  DECLARE_VST_DEPRECATED(audioMasterSetOutputSampleRate),
  DECLARE_VST_DEPRECATED(audioMasterGetOutputSpeakerArrangement),
  audioMasterGetVendorString,
  audioMasterGetProductString,
  audioMasterGetVendorVersion,
  audioMasterVendorSpecific,
  DECLARE_VST_DEPRECATED(audioMasterSetIcon),
  audioMasterCanDo,
  audioMasterGetLanguage,
  DECLARE_VST_DEPRECATED(audioMasterOpenWindow),
  DECLARE_VST_DEPRECATED(audioMasterCloseWindow),
  audioMasterGetDirectory,
  audioMasterUpdateDisplay,
  audioMasterBeginEdit,
  audioMasterEndEdit,
  audioMasterOpenFileSelector,
  audioMasterCloseFileSelector,
  DECLARE_VST_DEPRECATED(audioMasterEditFile),
  DECLARE_VST_DEPRECATED(audioMasterGetChunkFile),
  DECLARE_VST_DEPRECATED(audioMasterGetInputSpeakerArrangement),
};
