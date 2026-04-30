//------------------------------------------------------------------------
// Project     : VST SDK
// Version     : 2.4
//
// Category    : VST 2.x Classes
// Filename    : public.sdk/source/vst2.x/aeffeditor.h
// Created by  : Steinberg, 01/2004
// Description : Editor Class for VST Plug-Ins
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

#pragma once

#include "audioeffectx.h"

//-------------------------------------------------------------------------------------------------------
/** VST Effect Editor class. */
//-------------------------------------------------------------------------------------------------------
class AEffEditor
{
public:
//-------------------------------------------------------------------------------------------------------
	AEffEditor (AudioEffect* effect = 0)	///< Editor class constructor. Requires pointer to associated effect instance.
	: effect (effect)
	, systemWindow (0)
	{}

	virtual ~AEffEditor () ///< Editor class destructor.
	{}

	virtual AudioEffect* getEffect ()	{ return effect; }					///< Returns associated effect instance
	virtual bool getRect (ERect** rect)	{ *rect = 0; return false; }		///< Query editor size as #ERect
	virtual bool open (void* ptr)		{ systemWindow = ptr; return 0; }	///< Open editor, pointer to parent windows is platform-dependent (HWND on Windows, WindowRef on Mac).
	virtual void close ()				{ systemWindow = 0; }				///< Close editor (detach from parent window)
	virtual bool isOpen ()				{ return systemWindow != 0; }		///< Returns true if editor is currently open
	virtual void idle ()				{}									///< Idle call supplied by Host application

#if TARGET_API_MAC_CARBON
	virtual void DECLARE_VST_DEPRECATED (draw) (ERect* rect) {}
	virtual VstInt32 DECLARE_VST_DEPRECATED (mouse) (VstInt32 x, VstInt32 y) { return 0; }
	virtual VstInt32 DECLARE_VST_DEPRECATED (key) (VstInt32 keyCode) { return 0; }
	virtual void DECLARE_VST_DEPRECATED (top) () {}
	virtual void DECLARE_VST_DEPRECATED (sleep) () {}
#endif

#if VST_2_1_EXTENSIONS
	virtual bool onKeyDown (VstKeyCode& keyCode)	{ return false; }		///< Receive key down event. Return true only if key was really used!
	virtual bool onKeyUp (VstKeyCode& keyCode)		{ return false; }		///< Receive key up event. Return true only if key was really used!
	virtual bool onWheel (float distance)			{ return false; }		///< Handle mouse wheel event, distance is positive or negative to indicate wheel direction.
	virtual bool setKnobMode (VstInt32 val)			{ return false; }		///< Set knob mode (if supported by Host). See CKnobMode in VSTGUI.
#endif

//-------------------------------------------------------------------------------------------------------
protected:
	AudioEffect* effect;	///< associated effect instance
	void* systemWindow;		///< platform-dependent parent window (HWND or WindowRef)
};
