//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/vst2wrapper.h
// Created by  : Steinberg, 01/2009
// Description : VST 3 -> VST 2 Wrapper
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2019, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#pragma once

#include "docvst2.h"

/// \cond ignore
#include "basewrapper.h"
// #include "public.sdk/source/vst2.x/audioeffectx.h"

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

class Vst2MidiEventQueue;
class Vst2EditorWrapper;

//------------------------------------------------------------------------
class Vst2Wrapper : public BaseWrapper, public IVst3ToVst2Wrapper
// , public ::AudioEffectX, public IVst3ToVst2Wrapper
{
public:

 Vst2EditorWrapper *editor;

	// will owned factory
	static Vst2Wrapper* create (IPluginFactory* factory, const TUID vst3ComponentID, VstInt32 vst2ID, audioMasterCallback audioMaster);

	Vst2Wrapper (BaseWrapper::SVST3Config& config, audioMasterCallback audioMaster, VstInt32 vst2ID);
	virtual ~Vst2Wrapper ();

	//--- ------------------------------------------------------
	//--- BaseWrapper ------------------------------------------
	bool init ();
	void _canDoubleReplacing (bool val);
	void _setInitialDelay (int32 delay);
	void _noTail (bool val);
	void setupProcessTimeInfo ();

	void setupParameters ();
	void setupBuses ();

	void _ioChanged ();
	void _updateDisplay ();
	void _setNumInputs (int32 inputs);
	void _setNumOutputs (int32 outputs);
	bool _sizeWindow (int32 width, int32 height);

	//--- ---------------------------------------------------------------------
	// VST 3 Interfaces  ------------------------------------------------------
	// IComponentHandler
	tresult PLUGIN_API beginEdit (ParamID tag);
	tresult PLUGIN_API performEdit (ParamID tag, ParamValue valueNormalized);
	tresult PLUGIN_API endEdit (ParamID tag);

	// IHostApplication
	tresult PLUGIN_API getName (String128 name);

	//--- ---------------------------------------------------------------------
	// VST 2 AudioEffectX overrides -----------------------------------------------
	void suspend (); // Called when Plug-in is switched to off
	void resume (); // Called when Plug-in is switched to on
	VstInt32 startProcess ();
	VstInt32 stopProcess ();

	// Called when the sample rate changes (always in a suspend state)
	void setSampleRate (float newSamplerate);

	// Called when the maximum block size changes
	// (always in a suspend state). Note that the
	// sampleFrames in Process Calls could be
	// smaller than this block size, but NOT bigger.
	void setBlockSize (VstInt32 newBlockSize);

	float getParameter (VstInt32 index);
	void setParameter (VstInt32 index, float value);

	void setProgram (VstInt32 program);
	void setProgramName (char* name);
	VstInt32 getProgram ();
	void getProgramName (char* name);
	bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);

	void getParameterLabel (VstInt32 index, char* label);
	void getParameterDisplay (VstInt32 index, char* text);
	void getParameterName (VstInt32 index, char* text);
	bool canParameterBeAutomated (VstInt32 index);
	bool string2parameter (VstInt32 index, char* text);

	VstInt32 getChunk (void** data, bool isPreset = false);
	VstInt32 setChunk (void* data, VstInt32 byteSize, bool isPreset = false);

	bool setBypass (bool onOff);

	bool setProcessPrecision (VstInt32 precision);
	VstInt32 getNumMidiInputChannels ();
	VstInt32 getNumMidiOutputChannels ();
	VstInt32 getGetTailSize ();
	bool getEffectName (char* name);
	bool getVendorString (char* text);
	VstInt32 getVendorVersion ();
	VstIntPtr vendorSpecific (VstInt32 lArg, VstIntPtr lArg2, void* ptrArg,
		float floatArg);

	VstInt32 canDo (char* text);


	// finally process...
	void processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames);
	void processDoubleReplacing (double** inputs, double** outputs,
		VstInt32 sampleFrames);
	VstInt32 processEvents (VstEvents* events);
	
#ifndef VESTIGE
	bool getInputProperties (VstInt32 index, VstPinProperties* properties);
	bool getOutputProperties (VstInt32 index, VstPinProperties* properties);
	
	bool getPinProperties (BusDirection dir, VstInt32 pinIndex, VstPinProperties* properties);
	bool pinIndexToBusChannel (BusDirection dir, VstInt32 pinIndex, int32& busIndex, int32& busChannel);
	
	static VstInt32 vst3ToVst2SpeakerArr (SpeakerArrangement vst3Arr);
	static SpeakerArrangement vst2ToVst3SpeakerArr (VstInt32 vst2Arr);
	static VstInt32 vst3ToVst2Speaker (Speaker vst3Speaker);	
#endif	
	
	VstInt32 curProgram;
	int32 initialdelay;
	bool doublereplacing;
	int32 numinputs;
	int32 numoutputs;
	int32 numparams;
	int32 numprograms;
	bool synth;
	
	audioMasterCallback audioMaster;

	//--- ------------------------------------------------------
	DEFINE_INTERFACES
		DEF_INTERFACE (Vst::IVst3ToVst2Wrapper)
	END_DEFINE_INTERFACES (BaseWrapper)
	REFCOUNT_METHODS (BaseWrapper)	//------------------------------------------------------------------------
protected:
	Vst2MidiEventQueue* mVst2OutputEvents {nullptr};
	VstInt32 mCurrentProcessLevel {0};

	void updateProcessLevel ();

	void processOutputEvents ();

};

//------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

/** Must be implemented externally. */
// extern ::AudioEffect* createEffectInstance (audioMasterCallback audioMaster);

/// \endcond
