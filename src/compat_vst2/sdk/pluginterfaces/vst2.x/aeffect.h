#pragma once

#ifndef TARGET_API_MAC_CARBON
#define TARGET_API_MAC_CARBON 0
#endif

// Keep the SDK include path, but use a complete, pinned binary interface.
#include "../../../../../third_party/vst2-interface/vst2.h"

// The legacy SDK helpers declare these names as namespace constants.
#undef canDoSendVstEvents
#undef canDoSendVstMidiEvent
#undef canDoSendVstTimeInfo
#undef canDoReceiveVstEvents
#undef canDoReceiveVstMidiEvent
#undef canDoReportConnectionChanges
#undef canDoAcceptIOChanges
#undef canDoSizeWindow
#undef canDoOffline
#undef canDoOpenFileSelector
#undef canDoCloseFileSelector
#undef canDoStartStopProcess
#undef canDoShellCategory
#undef canDoSendVstMidiEventFlagIsRealtime
#undef canDoReceiveVstTimeInfo
#undef canDoMidiProgramNames
#undef canDoBypass

#include <cstddef>
static_assert(sizeof(AEffect) == 192);
static_assert(offsetof(AEffect, processReplacing) == 120);
static_assert(sizeof(VstPinProperties) == 128);
static_assert(offsetof(VstPinProperties, flags) == 64);
static_assert(offsetof(VstPinProperties, arrangementType) == 68);
static_assert(offsetof(VstPinProperties, shortLabel) == 72);
static_assert(audioMasterGetTime == 7 && audioMasterGetSampleRate == 16);
static_assert(audioMasterBeginEdit == 43 && audioMasterEndEdit == 44);
static_assert(kVstTransportChanged == 1 && kVstTransportPlaying == 2);
static_assert(kSpeakerArr51 == 15);
