//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/vst2wrapper.cpp
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

/// \cond ignore

#include "vst2wrapper.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/keycodes.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include "base/source/fstreamer.h"

#include <cstdio>
#include <cstdlib>
#include <limits>

extern bool DeinitModule (); //! Called in Vst2Wrapper destructor

//------------------------------------------------------------------------
// some Defines
//------------------------------------------------------------------------
// should be kVstMaxParamStrLen if we want to respect the VST 2 specification!!!
#define kVstExtMaxParamStrLen 32

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//! The parameter's name contains the unit path (e.g. "Modulators.LFO 1.frequency")
// bool vst2WrapperFullParameterPath = true;
bool vst2WrapperFullParameterPath = false;	

//------------------------------------------------------------------------
// Vst2EditorWrapper Declaration
//------------------------------------------------------------------------
class Vst2EditorWrapper : public BaseEditorWrapper
{
public:
//------------------------------------------------------------------------
	Vst2EditorWrapper (IEditController* controller, audioMasterCallback audioMaster);

	//--- from BaseEditorWrapper ---------------------
	void _close ();

	//--- from AEffEditor-------------------
	bool getRect (ERect** rect);
	bool open (void* ptr);
	void close () { _close (); }
	bool setKnobMode (VstInt32 val) 
	{
		return BaseEditorWrapper::_setKnobMode (static_cast<Vst::KnobMode> (val));
	}

	//--- IPlugFrame ----------------------------
	tresult PLUGIN_API resizeView (IPlugView* view, ViewRect* newSize);
	
	audioMasterCallback audioMaster3;

//------------------------------------------------------------------------
protected:
	ERect mERect;
};

//------------------------------------------------------------------------
bool areSizeEquals (const ViewRect &r1, const ViewRect& r2)
{
	if (r1.getHeight() != r2.getHeight ())
		return false;
	if (r1.getWidth() != r2.getWidth ())
		return false;
	return true;
}
	
#pragma GCC optimize ("O0")	

//------------------------------------------------------------------------
// Vst2EditorWrapper Implementation
//------------------------------------------------------------------------
Vst2EditorWrapper::Vst2EditorWrapper (IEditController* controller, audioMasterCallback audioMaster) : BaseEditorWrapper (controller)
{
	audioMaster3 = audioMaster;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Vst2EditorWrapper::resizeView (IPlugView* view, ViewRect* newSize)
{
AEffect plugin;
	
    BaseEditorWrapper::resizeView (view, newSize);
	
	if (audioMaster3)
	audioMaster3 (&plugin, audioMasterSizeWindow, newSize->getWidth (), newSize->getHeight (), 0, 0);    
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::getRect (ERect** rect)
{
	ViewRect size;
	if (BaseEditorWrapper::getRect (size))
	{
		mERect.left = (VstInt16)size.left;
		mERect.top = (VstInt16)size.top;
		mERect.right = (VstInt16)size.right;
		mERect.bottom = (VstInt16)size.bottom;

		*rect = &mERect;
		return true;
	}

	*rect = nullptr;
	return false;
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::open (void* ptr)
{
	return BaseEditorWrapper::_open (ptr);
}

//------------------------------------------------------------------------
void Vst2EditorWrapper::_close ()
{
	BaseEditorWrapper::_close ();
}
	
#pragma GCC optimize ("O2")	

//------------------------------------------------------------------------
// Vst2MidiEventQueue Declaration
//------------------------------------------------------------------------
class Vst2MidiEventQueue
{
public:
//------------------------------------------------------------------------
	Vst2MidiEventQueue (int32 maxEventCount);
	~Vst2MidiEventQueue ();

	bool isEmpty () const { return eventList->numEvents == 0; }
	bool add (const VstMidiEvent& e);
	void flush ();

	operator VstEvents* () { return eventList; }
//------------------------------------------------------------------------
protected:
	VstEvents* eventList;
	int32 maxEventCount;
};

//------------------------------------------------------------------------
// Vst2MidiEventQueue Implementation
//------------------------------------------------------------------------
Vst2MidiEventQueue::Vst2MidiEventQueue (int32 _maxEventCount) : maxEventCount (_maxEventCount)
{
	eventList = (VstEvents*)new char[sizeof (VstEvents) + (maxEventCount - 2) * sizeof (VstEvent*)];
	eventList->numEvents = 0;
	eventList->reserved = 0;

	int32 eventSize = sizeof (VstMidiEvent);

	for (int32 i = 0; i < maxEventCount; i++)
	{
		char* eventBuffer = new char[eventSize];
		memset (eventBuffer, 0, eventSize);
		eventList->events[i] = (VstEvent*)eventBuffer;
	}
}

//------------------------------------------------------------------------
Vst2MidiEventQueue::~Vst2MidiEventQueue ()
{
	for (int32 i = 0; i < maxEventCount; i++)
		delete[] (char*) eventList->events[i];

	delete[] (char*) eventList;
}

//------------------------------------------------------------------------
bool Vst2MidiEventQueue::add (const VstMidiEvent& e)
{
	if (eventList->numEvents >= maxEventCount)
		return false;

	auto* dst = (VstMidiEvent*)eventList->events[eventList->numEvents++];
	memcpy (dst, &e, sizeof (VstMidiEvent));
	dst->type = kVstMidiType;
	dst->byteSize = sizeof (VstMidiEvent);
	return true;
}

//------------------------------------------------------------------------
void Vst2MidiEventQueue::flush ()
{
	eventList->numEvents = 0;
}	

//------------------------------------------------------------------------
// Vst2Wrapper
//------------------------------------------------------------------------
Vst2Wrapper::Vst2Wrapper (BaseWrapper::SVST3Config& config, audioMasterCallback audioMaster2, VstInt32 vst2ID) : BaseWrapper (config), editor(0), curProgram(0), initialdelay(0), doublereplacing(false), numinputs(0), numoutputs(0), numparams(0), numprograms(0), synth(false)
{
	mUseExportedBypass = false;
	mUseIncIndex = true;
	
	audioMaster = audioMaster2;

//	setUniqueID (vst2ID);
//	canProcessReplacing (true); // supports replacing output
//	programsAreChunks (true);
}

//------------------------------------------------------------------------
Vst2Wrapper::~Vst2Wrapper ()
{
	//! editor needs to be destroyed BEFORE DeinitModule. Therefore destroy it here already
	//  instead of AudioEffect destructor
	if (mEditor)
	{
		mEditor = nullptr;
	}

	delete mVst2OutputEvents;
	mVst2OutputEvents = nullptr;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::init (audioMasterCallback audioMaster)
{
//	if (strstr (mSubCategories, "Instrument"))
//	synth = true;
//		isSynth (true);

    bool res = BaseWrapper::init ();

    numprograms = mNumPrograms;

	if (mController)
	{
		if (BaseEditorWrapper::hasEditor (mController))
		{
		editor = new Vst2EditorWrapper (mController, audioMaster);		
		if(editor)
		_setEditor (editor);
		}
	}
	return res;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_canDoubleReplacing (bool val)
{
//	canDoubleReplacing (val);
doublereplacing = val;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_setInitialDelay (int32 delay)
{
//	setInitialDelay (delay);
	initialdelay = delay;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_noTail (bool val)
{
//	noTail (val);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setupBuses ()
{
	BaseWrapper::setupBuses ();

	if (mHasEventOutputBuses)
	{
		if (mVst2OutputEvents == nullptr)
			mVst2OutputEvents = new Vst2MidiEventQueue (kMaxEvents);
	}
	else
	{
		if (mVst2OutputEvents)
		{
			delete mVst2OutputEvents;
			mVst2OutputEvents = nullptr;
		}
	}
}
//------------------------------------------------------------------------
void Vst2Wrapper::setupProcessTimeInfo ()
{
VstTimeInfo* vst2timeInfo;

vst2timeInfo = nullptr;

	if (audioMaster)
	vst2timeInfo = (VstTimeInfo*)audioMaster (0, audioMasterGetTime, 0, 0, 0, 0);

	if (vst2timeInfo)
	{
		const uint32 portableFlags =
		    ProcessContext::kPlaying | ProcessContext::kCycleActive | ProcessContext::kRecording |
		    ProcessContext::kSystemTimeValid | ProcessContext::kProjectTimeMusicValid |
		    ProcessContext::kBarPositionValid | ProcessContext::kCycleValid |
		    ProcessContext::kTempoValid | ProcessContext::kTimeSigValid |
		    ProcessContext::kSmpteValid | ProcessContext::kClockValid;

		mProcessContext.state = ((uint32)vst2timeInfo->flags) & portableFlags;
		mProcessContext.sampleRate = vst2timeInfo->sampleRate;
		mProcessContext.projectTimeSamples = (TSamples)vst2timeInfo->samplePos;

		if (mProcessContext.state & ProcessContext::kSystemTimeValid)
			mProcessContext.systemTime = (TSamples)vst2timeInfo->nanoSeconds;
		else
			mProcessContext.systemTime = 0;

		if (mProcessContext.state & ProcessContext::kProjectTimeMusicValid)
			mProcessContext.projectTimeMusic = vst2timeInfo->ppqPos;
		else
			mProcessContext.projectTimeMusic = 0;

		if (mProcessContext.state & ProcessContext::kBarPositionValid)
			mProcessContext.barPositionMusic = vst2timeInfo->barStartPos;
		else
			mProcessContext.barPositionMusic = 0;

		if (mProcessContext.state & ProcessContext::kCycleValid)
		{
			mProcessContext.cycleStartMusic = vst2timeInfo->cycleStartPos;
			mProcessContext.cycleEndMusic = vst2timeInfo->cycleEndPos;
		}
		else
			mProcessContext.cycleStartMusic = mProcessContext.cycleEndMusic = 0.0;

		if (mProcessContext.state & ProcessContext::kTempoValid)
			mProcessContext.tempo = vst2timeInfo->tempo;
		else
			mProcessContext.tempo = 120.0;

		if (mProcessContext.state & ProcessContext::kTimeSigValid)
		{
			mProcessContext.timeSigNumerator = vst2timeInfo->timeSigNumerator;
			mProcessContext.timeSigDenominator = vst2timeInfo->timeSigDenominator;
		}
		else
			mProcessContext.timeSigNumerator = mProcessContext.timeSigDenominator = 4;

		mProcessContext.frameRate.flags = 0;
		
		/*
		if (mProcessContext.state & ProcessContext::kSmpteValid)
		{
			mProcessContext.smpteOffsetSubframes = vst2timeInfo->smpteOffset;
			switch (vst2timeInfo->smpteFrameRate)
			{
				case kVstSmpte24fps: ///< 24 fps
					mProcessContext.frameRate.framesPerSecond = 24;
					break;
				case kVstSmpte25fps: ///< 25 fps
					mProcessContext.frameRate.framesPerSecond = 25;
					break;
				case kVstSmpte2997fps: ///< 29.97 fps
					mProcessContext.frameRate.framesPerSecond = 30;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte30fps: ///< 30 fps
					mProcessContext.frameRate.framesPerSecond = 30;
					break;
				case kVstSmpte2997dfps: ///< 29.97 drop
					mProcessContext.frameRate.framesPerSecond = 30;
					mProcessContext.frameRate.flags =
					    FrameRate::kPullDownRate | FrameRate::kDropRate;
					break;
				case kVstSmpte30dfps: ///< 30 drop
					mProcessContext.frameRate.framesPerSecond = 30;
					mProcessContext.frameRate.flags = FrameRate::kDropRate;
					break;
				case kVstSmpteFilm16mm: // not a smpte rate
				case kVstSmpteFilm35mm:
					mProcessContext.state &= ~ProcessContext::kSmpteValid;
					break;
				case kVstSmpte239fps: ///< 23.9 fps
					mProcessContext.frameRate.framesPerSecond = 24;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte249fps: ///< 24.9 fps
					mProcessContext.frameRate.framesPerSecond = 25;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte599fps: ///< 59.9 fps
					mProcessContext.frameRate.framesPerSecond = 60;
					mProcessContext.frameRate.flags = FrameRate::kPullDownRate;
					break;
				case kVstSmpte60fps: ///< 60 fps
					mProcessContext.frameRate.framesPerSecond = 60;
					break;
				default: mProcessContext.state &= ~ProcessContext::kSmpteValid; break;
			}
		}
		else
		{
		*/
			mProcessContext.smpteOffsetSubframes = 0;
			mProcessContext.frameRate.framesPerSecond = 0;
	//	}

		///< MIDI Clock Resolution (24 Per Quarter Note), can be negative (nearest)
	//	if (mProcessContext.state & ProcessContext::kClockValid)
	//		mProcessContext.samplesToNextClock = vst2timeInfo->samplesToNextClock;
	//	else
			mProcessContext.samplesToNextClock = 0;

		mProcessData.processContext = &mProcessContext;
	}
	else
		mProcessData.processContext = nullptr;
}

//------------------------------------------------------------------------
void Vst2Wrapper::suspend ()
{
	BaseWrapper::_suspend ();
}

//------------------------------------------------------------------------
void Vst2Wrapper::resume ()
{
	BaseWrapper::_resume ();
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::startProcess ()
{
	BaseWrapper::_startProcess ();
	return 0;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::stopProcess ()
{
	BaseWrapper::_stopProcess ();
	return 0;
}

//------------------------------------------------------------------------
void Vst2Wrapper::setSampleRate (float newSamplerate)
{
	BaseWrapper::_setSampleRate (newSamplerate);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setBlockSize (VstInt32 newBlockSize)
{
    BaseWrapper::_setBlockSize (newBlockSize);
}

//------------------------------------------------------------------------
float Vst2Wrapper::getParameter (VstInt32 index)
{
	return BaseWrapper::_getParameter (index);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setParameter (VstInt32 index, float value)
{
	if (!mController)
		return;

	if (index < (int32)mParameterMap.size ())
	{
		ParamID id = mParameterMap.at (index).vst3ID;
		addParameterChange (id, (ParamValue)value, 0);
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::setProgram (VstInt32 program)
{
	if (mProgramParameterID != kNoParamId && mController != nullptr && mProgramParameterIdx != -1)
	{
        curProgram = program;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (mProgramParameterIdx, paramInfo) == kResultTrue)
		{
			if (paramInfo.stepCount > 0 && program <= paramInfo.stepCount)
			{
				ParamValue normalized = (ParamValue)program / (ParamValue)paramInfo.stepCount;
				addParameterChange (mProgramParameterID, normalized, 0);
			}
		}
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::setProgramName (char* name)
{
	// not supported in VST 3
}

//------------------------------------------------------------------------
void Vst2Wrapper::getProgramName (char* name)
{
	// name of the current program. Limited to #kVstMaxProgNameLen.
	*name = 0;
	if (mUnitInfo)
	{
		ProgramListInfo listInfo = {0};
		if (mUnitInfo->getProgramListInfo (0, listInfo) == kResultTrue)
		{
			String128 tmp = {0};
			if (mUnitInfo->getProgramName (listInfo.id, curProgram, tmp) == kResultTrue)
			{
				String str (tmp);
				str.copyTo8 (name, 0, kVstMaxProgNameLen);
			}
		}
	}
}

int Vst2Wrapper::getProgram ()
{
    return curProgram;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getProgramNameIndexed (VstInt32, VstInt32 index, char* name)
{
	*name = 0;
	if (mUnitInfo)
	{
		ProgramListInfo listInfo = {0};
		if (mUnitInfo->getProgramListInfo (0, listInfo) == kResultTrue)
		{
			String128 tmp = {0};
			if (mUnitInfo->getProgramName (listInfo.id, index, tmp) == kResultTrue)
			{
				String str (tmp);
				str.copyTo8 (name, 0, kVstMaxProgNameLen);
				return true;
			}
		}
	}

	return false;
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterLabel (VstInt32 index, char* label)
{
	// units in which parameter \e index is displayed (i.e. "sec", "dB", "type", etc...). Limited to
	// #kVstMaxParamStrLen.
	*label = 0;
	if (mController)
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String str (paramInfo.units);
			str.copyTo8 (label, 0, kVstMaxParamStrLen);
		}
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterDisplay (VstInt32 index, char* text)
{
	// string representation ("0.5", "-3", "PLATE", etc...) of the value of parameter \e index.
	// Limited to #kVstMaxParamStrLen.
	*text = 0;
	if (mController)
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String128 tmp = {0};
			ParamValue value = 0;
			if (!getLastParamChange (paramInfo.id, value))
				value = mController->getParamNormalized (paramInfo.id);

			if (mController->getParamStringByValue (paramInfo.id, value, tmp) == kResultTrue)
			{
				String str (tmp);
				str.copyTo8 (text, 0, kVstMaxParamStrLen);
			}
		}
	}
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterName (VstInt32 index, char* text)
{
	// name ("Time", "Gain", "RoomType", etc...) of parameter \e index. Limited to
	// #kVstExtMaxParamStrLen.
	*text = 0;
	if (mController && index < (int32)mParameterMap.size ())
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			String str;
			if (vst2WrapperFullParameterPath)
			{
				//! The parameter's name contains the unit path (e.g. "LFO 1.freq") as well
				if (mUnitInfo)
				{
					getUnitPath (paramInfo.unitId, str);
				}
			}
			str.append (paramInfo.title);

			if (str.length () > kVstExtMaxParamStrLen)
			{
				//! In case the string's length exceeds the limit, try parameter's title without
				// unit path.
				str = paramInfo.title;
			}
			if (str.length () > kVstExtMaxParamStrLen)
			{
				str = paramInfo.shortTitle;
			}
			str.copyTo8 (text, 0, kVstExtMaxParamStrLen);
		}
	}
}

//------------------------------------------------------------------------
bool Vst2Wrapper::canParameterBeAutomated (VstInt32 index)
{
	if (mController && index < (int32)mParameterMap.size ())
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
			return (paramInfo.flags & ParameterInfo::kCanAutomate) != 0;
	}
	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::string2parameter (VstInt32 index, char* text)
{
	if (mController && index < (int32)mParameterMap.size ())
	{
		int32 vst3Index = mParameterMap.at (index).vst3Index;

		ParameterInfo paramInfo = {0};
		if (mController->getParameterInfo (vst3Index, paramInfo) == kResultTrue)
		{
			TChar tString[1024] = {0};
			String tmp (text);
			tmp.copyTo16 (tString, 0, 1023);

			ParamValue valueNormalized = 0.0;

			if (mController->getParamValueByString (paramInfo.id, tString, valueNormalized))
			{
			
		setParameter (index, (float)valueNormalized);
	    if (audioMaster)
		audioMaster (0, audioMasterAutomate, index, 0, 0, (float)valueNormalized);	
			//	setParameterAutomated (index, (float)valueNormalized);
				// TODO: check if setParameterAutomated is correct
			}
		}
	}
	return false;
}



//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getChunk (void** data, bool isPreset)
{
	return BaseWrapper::_getChunk (data, isPreset);
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::setChunk (void* data, VstInt32 byteSize, bool isPreset)
{
	return BaseWrapper::_setChunk (data, byteSize, isPreset);
}


//------------------------------------------------------------------------
bool Vst2Wrapper::setBypass (bool onOff)
{
	return BaseWrapper::_setBypass (onOff);
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::setProcessPrecision (VstInt32 precision)
{
	int32 newVst3SampleSize = -1;

	if (precision == kVstProcessPrecision32)
		newVst3SampleSize = kSample32;
	else if (precision == kVstProcessPrecision64)
		newVst3SampleSize = kSample64;

	if (newVst3SampleSize != mVst3SampleSize)
	{
		if (mProcessor && mProcessor->canProcessSampleSize (newVst3SampleSize) == kResultTrue)
		{
			mVst3SampleSize = newVst3SampleSize;
			setupProcessing ();

			setupBuses ();

			return true;
		}
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getNumMidiInputChannels ()
{
	if (!mComponent)
		return 0;

	int32 busCount = mComponent->getBusCount (kEvent, kInput);
	if (busCount > 0)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kEvent, kInput, 0, busInfo) == kResultTrue)
		{
			return busInfo.channelCount;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getNumMidiOutputChannels ()
{
	if (!mComponent)
		return 0;

	int32 busCount = mComponent->getBusCount (kEvent, kOutput);
	if (busCount > 0)
	{
		BusInfo busInfo = {0};
		if (mComponent->getBusInfo (kEvent, kOutput, 0, busInfo) == kResultTrue)
		{
			return busInfo.channelCount;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getGetTailSize ()
{
	if (mProcessor)
		return mProcessor->getTailSamples ();

	return 0;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getEffectName (char* effectName)
{
	if (mName[0])
	{
		strncpy (effectName, mName, kVstMaxEffectNameLen);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getVendorString (char* text)
{
	if (mVendor[0])
	{
		strncpy (text, mVendor, kVstMaxVendorStrLen);
		return true;
	}
	return false;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getVendorVersion ()
{
	return mVersion;
}

//-----------------------------------------------------------------------------
VstIntPtr Vst2Wrapper::vendorSpecific (VstInt32 lArg, VstIntPtr lArg2, void* ptrArg, float floatArg)
{
	switch (lArg)
	{
		case 'stCA':
		case 'stCa':
			switch (lArg2)
			{
				//--- -------
				case 'FUID':
					if (ptrArg && mVst3EffectClassID.isValid ())
					{
						memcpy ((char*)ptrArg, mVst3EffectClassID, 16);
						return 1;
					}
					break;

			}
	}
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::canDo (char* text)
{
	if (stricmp (text, "sendVstEvents") == 0)
	{
		return -1;
	}
	else if (stricmp (text, "sendVstMidiEvent") == 0)
	{
		return mHasEventOutputBuses ? 1 : -1;
	}
	else if (stricmp (text, "receiveVstEvents") == 0)
	{
		return -1;
	}
	else if (stricmp (text, "receiveVstMidiEvent") == 0)
	{
		return mHasEventInputBuses ? 1 : -1;
	}
	else if (stricmp (text, "receiveVstTimeInfo") == 0)
	{
		return 1;
	}
	else if (stricmp (text, "offline") == 0)
	{
		if (mProcessing)
			return 0;
		if (mVst3processMode == kOffline)
			return 1;

		bool canOffline = setupProcessing (kOffline);
		setupProcessing ();
		return canOffline ? 1 : -1;
	}
	else if (stricmp (text, "midiProgramNames") == 0)
	{
		if (mUnitInfo)
		{
			UnitID unitId = -1;
			if (mUnitInfo->getUnitByBus (kEvent, kInput, 0, 0, unitId) == kResultTrue &&
			    unitId >= 0)
				return 1;
		}
		return -1;
	}
	else if (stricmp (text, "bypass") == 0)
	{
		return mBypassParameterID != kNoParamId ? 1 : -1;
	}
	return 0; // do not know
}




//-----------------------------------------------------------------------------
void Vst2Wrapper::setupParameters ()
{
	BaseWrapper::setupParameters ();
//	cEffect.numParams = mNumParams;
   numparams = mNumParams;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::processEvents (VstEvents* events)
{
	if (mInputEvents == nullptr)
		return 0;
	mInputEvents->clear ();

	for (int32 i = 0; i < events->numEvents; i++)
	{
		VstEvent* e = events->events[i];
		if (e->type == kVstMidiType)
		{
			auto* midiEvent = (VstMidiEvent*)e;
			Event toAdd = {0, midiEvent->deltaFrames, 0};
			processMidiEvent (toAdd, &midiEvent->midiData[0],
			                  midiEvent->flags & kVstMidiEventIsRealtime, midiEvent->noteLength,
			                  midiEvent->noteOffVelocity * kMidiScaler, midiEvent->detune);
		}
		//--- -----------------------------
	}

	return 0;
}

//-----------------------------------------------------------------------------
inline void Vst2Wrapper::processOutputEvents ()
{
	if (mVst2OutputEvents && mOutputEvents && mOutputEvents->getEventCount () > 0)
	{
		mVst2OutputEvents->flush ();

		Event e = {0};
		for (int32 i = 0, total = mOutputEvents->getEventCount (); i < total; i++)
		{
			if (mOutputEvents->getEvent (i, e) != kResultOk)
				break;

				VstMidiEvent midiEvent = {0};
				midiEvent.deltaFrames = e.sampleOffset;
				if (e.flags & Event::kIsLive)
					midiEvent.flags = kVstMidiEventIsRealtime;
				char* midiData = midiEvent.midiData;

				switch (e.type)
				{
					//--- ---------------------
					case Event::kNoteOnEvent:
						midiData[0] = (char)(kNoteOn | (e.noteOn.channel & kChannelMask));
						midiData[1] = (char)(e.noteOn.pitch & kDataMask);
						midiData[2] =
						    (char)((int32) (e.noteOn.velocity * 127.f + 0.4999999f) & kDataMask);
						if (midiData[2] == 0) // zero velocity => note off
							midiData[0] = (char)(kNoteOff | (e.noteOn.channel & kChannelMask));
						midiEvent.detune = (char)e.noteOn.tuning;
						midiEvent.noteLength = e.noteOn.length;
						break;

					//--- ---------------------
					case Event::kNoteOffEvent:
						midiData[0] = (char)(kNoteOff | (e.noteOff.channel & kChannelMask));
						midiData[1] = (char)(e.noteOff.pitch & kDataMask);
						midiData[2] = midiEvent.noteOffVelocity =
						    (char)((int32) (e.noteOff.velocity * 127.f + 0.4999999f) & kDataMask);
						break;

						break;
				}

				if (!mVst2OutputEvents->add (midiEvent))
					break;
			}

		mOutputEvents->clear ();

	//	sendVstEventsToHost (*mVst2OutputEvents);
	
		if (audioMaster)
	    audioMaster (0, audioMasterProcessEvents, 0, 0, *mVst2OutputEvents, 0);
	}
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::updateProcessLevel ()
{
int32 currentLevel;

currentLevel = 0;

	if (audioMaster)
	currentLevel = audioMaster (0, audioMasterGetCurrentProcessLevel, 0, 0, 0, 0);

	// getCurrentProcessLevel ();
	if (mCurrentProcessLevel != currentLevel)
	{
		mCurrentProcessLevel = currentLevel;
	//	if (mCurrentProcessLevel == kVstProcessLevelOffline)
	//		mVst3processMode = kOffline;
	//	else
			mVst3processMode = kRealtime;

		bool callStartStop = mProcessing;

		if (callStartStop)
			stopProcess ();

		setupProcessing ();

		if (callStartStop)
			startProcess ();
	}
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::processReplacing (float** inputs, float** outputs, VstInt32 sampleFrames)
{
	updateProcessLevel ();

	_processReplacing (inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::processDoubleReplacing (double** inputs, double** outputs, VstInt32 sampleFrames)
{
	updateProcessLevel ();

	_processDoubleReplacing (inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------
// static
//-----------------------------------------------------------------------------
Vst2Wrapper* Vst2Wrapper::create (IPluginFactory* factory, const TUID vst3ComponentID, VstInt32 vst2ID, audioMasterCallback audioMaster)
{
	    if (!factory)
		return nullptr;

	    BaseWrapper::SVST3Config config;
    	config.factory = factory; 
    	config.processor = nullptr;	
        config.component = nullptr; 
 
        FReleaser factoryReleaser (factory); 
        
        config.factory = factory;
        
        config.vst3ComponentID = FUID::fromTUID (vst3ComponentID);

		auto* wrapper = new Vst2Wrapper (config, audioMaster, vst2ID);
		
		if(!wrapper)
		return nullptr;
		
	    if (wrapper->init (audioMaster) == false)
		{
			wrapper->release ();
			return nullptr;
		}
				
		FUnknownPtr<IPluginFactory2> factory2 (factory);
		if (factory2)
		{
			PFactoryInfo factoryInfo;
			if (factory2->getFactoryInfo (&factoryInfo) == kResultTrue)
				wrapper->setVendorName (factoryInfo.vendor);

			for (int32 i = 0; i < factory2->countClasses (); i++)
			{
				Steinberg::PClassInfo2 classInfo2;
				if (factory2->getClassInfo2 (i, &classInfo2) == Steinberg::kResultTrue)
				{
					if (memcmp (classInfo2.cid, vst3ComponentID, sizeof (TUID)) == 0)
					{
						wrapper->setSubCategories (classInfo2.subCategories);
						wrapper->setEffectName (classInfo2.name);
						wrapper->setEffectVersion (classInfo2.version);

						if (classInfo2.vendor[0] != 0)
							wrapper->setVendorName (classInfo2.vendor);

						break;
					}
				}
			}
		}
	
		if (strstr (wrapper->mSubCategories, "Instrument"))
	        wrapper->synth = true;
				
		return wrapper;
}
	
//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::getName (String128 name)
{
	char8 productString[128];
	int retval;
	
	retval = 0;
	
	if (audioMaster)
	retval = audioMaster (0, audioMasterGetProductString, 0, 0, productString, 0);
	
	//if (getHostProductString (productString))
	if(retval)
	{
		String str (productString);
		str.copyTo16 (name, 0, 127);

		return kResultTrue;
	}
	return kResultFalse;
}

//-----------------------------------------------------------------------------
// IComponentHandler
//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::beginEdit (ParamID tag)
{
	std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find (tag);
	if (iter != mParamIndexMap.end ())
	{
	if (audioMaster)
	audioMaster (0, audioMasterBeginEdit, (*iter).second, 0, 0, 0);	
	}
	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::performEdit (ParamID tag, ParamValue valueNormalized)
{
	std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find (tag);
	if (iter != mParamIndexMap.end () && audioMaster)
	{	
	if (audioMaster)
	audioMaster (0, audioMasterAutomate, (*iter).second, 0, nullptr, (float)valueNormalized); // value is in opt
	}

	mInputTransfer.addChange (tag, valueNormalized, 0);

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::endEdit (ParamID tag)
{
	std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find (tag);
	if (iter != mParamIndexMap.end ())
	{
	if (audioMaster)
	audioMaster (0, audioMasterEndEdit, (*iter).second, 0, 0, 0);
	
	}
	return kResultTrue;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_ioChanged ()
{
AEffect plugin;

	BaseWrapper::_ioChanged ();
	
        plugin.numPrograms = numprograms;
        plugin.numParams = numparams;
        plugin.numInputs = numinputs;
        plugin.numOutputs = numoutputs;
        plugin.initialDelay = initialdelay;
        
	if (audioMaster)
	audioMaster (&plugin, audioMasterIOChanged, 0, 0, 0, 0);       	
	
//	ioChanged ();
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_updateDisplay ()
{
	BaseWrapper::_updateDisplay ();
//	updateDisplay ();
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_setNumInputs (int32 inputs)
{
	BaseWrapper::_setNumInputs (inputs);
	
//	setNumInputs (inputs);
numinputs = inputs;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_setNumOutputs (int32 outputs)
{
	BaseWrapper::_setNumOutputs (outputs);
//	setNumOutputs (outputs);
numoutputs = outputs;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::_sizeWindow (int32 width, int32 height)
{
//	return sizeWindow (width, height);
}
	
#ifndef VESTIGE
//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::vst3ToVst2SpeakerArr (SpeakerArrangement vst3Arr)
{
	switch (vst3Arr)
	{
		case SpeakerArr::kMono: return kSpeakerArrMono;
		case SpeakerArr::kStereo: return kSpeakerArrStereo;
		case SpeakerArr::kStereoSurround: return kSpeakerArrStereoSurround;
		case SpeakerArr::kStereoCenter: return kSpeakerArrStereoCenter;
		case SpeakerArr::kStereoSide: return kSpeakerArrStereoSide;
		case SpeakerArr::kStereoCLfe: return kSpeakerArrStereoCLfe;
		case SpeakerArr::k30Cine: return kSpeakerArr30Cine;
		case SpeakerArr::k30Music: return kSpeakerArr30Music;
		case SpeakerArr::k31Cine: return kSpeakerArr31Cine;
		case SpeakerArr::k31Music: return kSpeakerArr31Music;
		case SpeakerArr::k40Cine: return kSpeakerArr40Cine;
		case SpeakerArr::k40Music: return kSpeakerArr40Music;
		case SpeakerArr::k41Cine: return kSpeakerArr41Cine;
		case SpeakerArr::k41Music: return kSpeakerArr41Music;
		case SpeakerArr::k50: return kSpeakerArr50;
		case SpeakerArr::k51: return kSpeakerArr51;
		case SpeakerArr::k60Cine: return kSpeakerArr60Cine;
		case SpeakerArr::k60Music: return kSpeakerArr60Music;
		case SpeakerArr::k61Cine: return kSpeakerArr61Cine;
		case SpeakerArr::k61Music: return kSpeakerArr61Music;
		case SpeakerArr::k70Cine: return kSpeakerArr70Cine;
		case SpeakerArr::k70Music: return kSpeakerArr70Music;
		case SpeakerArr::k71Cine: return kSpeakerArr71Cine;
		case SpeakerArr::k71Music: return kSpeakerArr71Music;
		case SpeakerArr::k80Cine: return kSpeakerArr80Cine;
		case SpeakerArr::k80Music: return kSpeakerArr80Music;
		case SpeakerArr::k81Cine: return kSpeakerArr81Cine;
		case SpeakerArr::k81Music: return kSpeakerArr81Music;
		case SpeakerArr::k102: return kSpeakerArr102;
	}

	return kSpeakerArrUserDefined;
}

//------------------------------------------------------------------------
SpeakerArrangement Vst2Wrapper::vst2ToVst3SpeakerArr (VstInt32 vst2Arr)
{
	switch (vst2Arr)
	{
		case kSpeakerArrMono: return SpeakerArr::kMono;
		case kSpeakerArrStereo: return SpeakerArr::kStereo;
		case kSpeakerArrStereoSurround: return SpeakerArr::kStereoSurround;
		case kSpeakerArrStereoCenter: return SpeakerArr::kStereoCenter;
		case kSpeakerArrStereoSide: return SpeakerArr::kStereoSide;
		case kSpeakerArrStereoCLfe: return SpeakerArr::kStereoCLfe;
		case kSpeakerArr30Cine: return SpeakerArr::k30Cine;
		case kSpeakerArr30Music: return SpeakerArr::k30Music;
		case kSpeakerArr31Cine: return SpeakerArr::k31Cine;
		case kSpeakerArr31Music: return SpeakerArr::k31Music;
		case kSpeakerArr40Cine: return SpeakerArr::k40Cine;
		case kSpeakerArr40Music: return SpeakerArr::k40Music;
		case kSpeakerArr41Cine: return SpeakerArr::k41Cine;
		case kSpeakerArr41Music: return SpeakerArr::k41Music;
		case kSpeakerArr50: return SpeakerArr::k50;
		case kSpeakerArr51: return SpeakerArr::k51;
		case kSpeakerArr60Cine: return SpeakerArr::k60Cine;
		case kSpeakerArr60Music: return SpeakerArr::k60Music;
		case kSpeakerArr61Cine: return SpeakerArr::k61Cine;
		case kSpeakerArr61Music: return SpeakerArr::k61Music;
		case kSpeakerArr70Cine: return SpeakerArr::k70Cine;
		case kSpeakerArr70Music: return SpeakerArr::k70Music;
		case kSpeakerArr71Cine: return SpeakerArr::k71Cine;
		case kSpeakerArr71Music: return SpeakerArr::k71Music;
		case kSpeakerArr80Cine: return SpeakerArr::k80Cine;
		case kSpeakerArr80Music: return SpeakerArr::k80Music;
		case kSpeakerArr81Cine: return SpeakerArr::k81Cine;
		case kSpeakerArr81Music: return SpeakerArr::k81Music;
		case kSpeakerArr102: return SpeakerArr::k102;
	}

	return 0;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::vst3ToVst2Speaker (Vst::Speaker vst3Speaker)
{
	switch (vst3Speaker)
	{
		case Vst::kSpeakerM: return ::kSpeakerM;
		case Vst::kSpeakerL: return ::kSpeakerL;
		case Vst::kSpeakerR: return ::kSpeakerR;
		case Vst::kSpeakerC: return ::kSpeakerC;
		case Vst::kSpeakerLfe: return ::kSpeakerLfe;
		case Vst::kSpeakerLs: return ::kSpeakerLs;
		case Vst::kSpeakerRs: return ::kSpeakerRs;
		case Vst::kSpeakerLc: return ::kSpeakerLc;
		case Vst::kSpeakerRc: return ::kSpeakerRc;
		case Vst::kSpeakerS: return ::kSpeakerS;
		case Vst::kSpeakerSl: return ::kSpeakerSl;
		case Vst::kSpeakerSr: return ::kSpeakerSr;
		case Vst::kSpeakerTc: return ::kSpeakerTm;
		case Vst::kSpeakerTfl: return ::kSpeakerTfl;
		case Vst::kSpeakerTfc: return ::kSpeakerTfc;
		case Vst::kSpeakerTfr: return ::kSpeakerTfr;
		case Vst::kSpeakerTrl: return ::kSpeakerTrl;
		case Vst::kSpeakerTrc: return ::kSpeakerTrc;
		case Vst::kSpeakerTrr: return ::kSpeakerTrr;
		case Vst::kSpeakerLfe2: return ::kSpeakerLfe2;
	}
	return ::kSpeakerUndefined;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::pinIndexToBusChannel (BusDirection dir, VstInt32 pinIndex, int32& busIndex,
                                        int32& busChannel)
{
	AudioBusBuffers* busBuffers = dir == kInput ? mProcessData.inputs : mProcessData.outputs;
	int32 busCount = dir == kInput ? mProcessData.numInputs : mProcessData.numOutputs;
	uint64 mainBusFlags = dir == kInput ? mMainAudioInputBuses : mMainAudioOutputBuses;

	int32 sourceIndex = 0;
	for (busIndex = 0; busIndex < busCount; busIndex++)
	{
		AudioBusBuffers& buffers = busBuffers[busIndex];
		if (mainBusFlags & (uint64 (1) << busIndex))
		{
			for (busChannel = 0; busChannel < buffers.numChannels; busChannel++)
			{
				if (pinIndex == sourceIndex)
				{
					return true;
				}
				sourceIndex++;
			}
		}
	}
	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getPinProperties (BusDirection dir, VstInt32 pinIndex,
                                    VstPinProperties* properties)
{
	int32 busIndex = -1;
	int32 busChannelIndex = -1;

	if (pinIndexToBusChannel (dir, pinIndex, busIndex, busChannelIndex))
	{
		BusInfo busInfo = {0};
		if (mComponent && mComponent->getBusInfo (kAudio, dir, busIndex, busInfo) == kResultTrue)
		{
		    properties->flags = kVstPinIsActive; // ????

			String name (busInfo.name);
			name.copyTo8 (properties->label, 0, kVstMaxLabelLen);

			if (busInfo.channelCount == 1)
			{
				properties->flags |= kVstPinUseSpeaker;
				properties->arrangementType = kSpeakerArrMono;
			}
			if (busInfo.channelCount == 2)
			{
				properties->flags |= kVstPinUseSpeaker;
				properties->flags |= kVstPinIsStereo;
				properties->arrangementType = kSpeakerArrStereo;
			}
			else if (busInfo.channelCount > 2)
			{
				Vst::SpeakerArrangement arr = 0;
				if (mProcessor && mProcessor->getBusArrangement (dir, busIndex, arr) == kResultTrue)
				{
					properties->flags |= kVstPinUseSpeaker;
					properties->arrangementType = vst3ToVst2SpeakerArr (arr);
				}
				else
				{
					return false;
				}
			}
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getInputProperties (VstInt32 index, VstPinProperties* properties)
{
	return getPinProperties (kInput, index, properties);
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getOutputProperties (VstInt32 index, VstPinProperties* properties)
{
	return getPinProperties (kOutput, index, properties);
}
#endif
	
//-----------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

extern bool InitModule ();

//-----------------------------------------------------------------------------

/// \endcond
