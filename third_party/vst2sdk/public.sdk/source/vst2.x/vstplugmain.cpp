//------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 2.4
//
// Category    : VST 2.x Classes
// Filename    : public.sdk/source/vst2.x/vstplugmain.cpp
// Created by  : Steinberg, 01/2004
// Description : VST Plug-In Main Entry
// 
//-----------------------------------------------------------------------------
// LICENSE
//-----------------------------------------------------------------------------
// Steinberg VST technology is open source and available under the MIT License.
// The MIT License allows the use, modification and distribution of software
// provided the original copyright notice and license text are retained in the
// distributed software. This correctly attributes the work of the original
// authors and allows developers to legally use corresponding technologies.
// This license allows free and unlimited use of the VST technology for the
// development of software products and their commercial distribution.
//----------------------------------------------------------------------------------

#include "audioeffect.h"

//------------------------------------------------------------------------
/** Must be implemented externally. */
extern AudioEffect* createEffectInstance (audioMasterCallback audioMaster);

extern "C" {

#if defined (__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
	#define VST_EXPORT	__attribute__ ((visibility ("default")))
#else
	#define VST_EXPORT
#endif

//------------------------------------------------------------------------
/** Prototype of the export function main */
//------------------------------------------------------------------------
VST_EXPORT AEffect* VSTPluginMain (audioMasterCallback audioMaster)
{
	// Get VST Version of the Host
	if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
		return 0;  // old version

	// Create the AudioEffect
	AudioEffect* effect = createEffectInstance (audioMaster);
	if (!effect)
		return 0;

	// Return the VST AEffect structur
	return effect->getAeffect ();
}

// support for old hosts not looking for VSTPluginMain
#if (TARGET_API_MAC_CARBON && __ppc__)
VST_EXPORT AEffect* main_macho (audioMasterCallback audioMaster) { return VSTPluginMain (audioMaster); }
#elif WIN32
VST_EXPORT AEffect* MAIN (audioMasterCallback audioMaster) { return VSTPluginMain (audioMaster); }
#elif BEOS
VST_EXPORT AEffect* main_plugin (audioMasterCallback audioMaster) { return VSTPluginMain (audioMaster); }
#endif

} // extern "C"

//------------------------------------------------------------------------
#if WIN32
#include <windows.h>
void* hInstance;

extern "C" {
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
	hInstance = hInst;
	return 1;
}
} // extern "C"
#endif
