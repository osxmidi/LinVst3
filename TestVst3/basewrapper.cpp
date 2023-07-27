//------------------------------------------------------------------------
// Project     : VST SDK
//
// Category    : Helpers
// Filename    : public.sdk/source/vst/vst2wrapper/basewrapper.cpp
// Created by  : Steinberg, 01/2009
// Description : VST 3 -> VST 2 Wrapper
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2018, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
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
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

/// \cond ignore

// wineg++ -c -std=c++14 -m64 -O2 -I../ -DVESTIGE -DRELEASE=1  -D__forceinline=inline -DNOMINMAX=1 -DUNICODE_OFF -Dstricmp=strcasecmp -Dstrnicmp=strncasecmp -DSMTG_RENAME_ASSERT=1 -fpermissive basewrapper.cpp


#include <stdint.h>

#ifdef VESTIGE
typedef int16_t VstInt16;
typedef int32_t VstInt32;
typedef int64_t VstInt64;
typedef intptr_t VstIntPtr;
#define VESTIGECALLBACK __cdecl
#include "vestige.h"
#else
#include "pluginterfaces/vst2.x/aeffectx.h"
#endif

#ifdef __WINE__
#else
#define __cdecl
#endif

#define WIN32_LEAN_AND_MEAN


#include "basewrapper.h"
#include "public.sdk/source/vst/hosting/hostclasses.h"

#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/base/keycodes.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/vstpresetkeys.h"

#include "base/source/fstreamer.h"



#include "vst2wrapper.h"

#include "public.sdk/source/vst/hosting/hostclasses.h"

#include "base/source/fstreamer.h"










#include <cstdio>
#include <cstdlib>
#include <limits>


//#include "public.sdk/source/vst/hosting/eventlist.cpp"
//#include "public.sdk/source/vst/hosting/hostclasses.cpp"
//#include "public.sdk/source/vst/hosting/parameterchanges.cpp"
//#include "public.sdk/source/vst/hosting/pluginterfacesupport.cpp"
//#include "public.sdk/source/vst/hosting/processdata.cpp"

//#include "public.sdk/source/common/memorystream.cpp"


bool InitModule() { return true; }

bool DeinitModule() { return true; }

//extern bool DeinitModule(); 

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//------------------------------------------------------------------------
// some Globals
//------------------------------------------------------------------------
// In order to speed up hasEditor function gPluginHasEditor can be set in
// EditController::initialize
enum {
  kDontKnow = -1,
  kNoEditor = 0,
  kEditor,
};

// default: kDontKnow which uses createView to find out
using EditorAvailability = int32;
EditorAvailability gPluginHasEditor = kDontKnow;

// Set to 'true' in EditController::initialize
// default: VST 3 kIsProgramChange parameter will not be exported
bool gExportProgramChangeParameters = false;

//------------------------------------------------------------------------
BaseEditorWrapper::BaseEditorWrapper(IEditController *controller)
    : mController(controller), mView(nullptr) {}

//------------------------------------------------------------------------
BaseEditorWrapper::~BaseEditorWrapper() {
  if (mView)
    _close();

  mController = nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API BaseEditorWrapper::queryInterface(const char *_iid,
                                                     void **obj) {
  QUERY_INTERFACE(_iid, obj, FUnknown::iid, IPlugFrame)
  //QUERY_INTERFACE(_iid, obj, IPlugFrame::iid, IPlugFrame)

  *obj = nullptr;
  return kNoInterface;
}

//------------------------------------------------------------------------
tresult PLUGIN_API BaseEditorWrapper::resizeView(IPlugView *view,
                                                 ViewRect *newSize) {
  tresult result = kResultFalse;
  if (view && newSize) {
    result = view->onSize(newSize);
  return result;
  }
  return false;
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::hasEditor(IEditController *controller) {
  /* Some Plug-ins might have large GUIs. In order to speed up hasEditor
   * function while initializing the Plug-in gPluginHasEditor can be set in
   * EditController::initialize beforehand. */
  bool result = false;
  if (gPluginHasEditor == kEditor) {
    result = true;
  } else if (gPluginHasEditor == kNoEditor) {
    result = false;
  } else {
    IPlugView *view =
        controller ? controller->createView(ViewType::kEditor) : nullptr;
    FReleaser viewRel(view);
    result = (view != nullptr);
  }

  return result;
}

//------------------------------------------------------------------------
void BaseEditorWrapper::createView() {
  if (mView == nullptr && mController != nullptr) {
    mView = owned(mController->createView(ViewType::kEditor));
    mView->setFrame(this);

#if SMTG_OS_MACOS
#if SMTG_PLATFORM_64
    if (mView &&
        mView->isPlatformTypeSupported(kPlatformTypeNSView) != kResultTrue)
#else
    if (mView &&
        mView->isPlatformTypeSupported(kPlatformTypeHIView) != kResultTrue)
#endif
    {
      mView = nullptr;
      mController = nullptr;
    }
#endif
  }
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::getRect(ViewRect &rect) {
  createView();
  if (!mView)
    return false;

  if (mView->getSize(&rect) == kResultTrue) {
    if ((rect.bottom - rect.top) > 0 && (rect.right - rect.left) > 0) {
      mViewRect = rect;
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::_open(void *ptr) {
  createView();

  if (mView) {
#if SMTG_OS_WINDOWS
    FIDString type = kPlatformTypeHWND;
#elif SMTG_OS_MACOS && SMTG_PLATFORM_64
    FIDString type = kPlatformTypeNSView;
#elif SMTG_OS_MACOS
    FIDString type = kPlatformTypeHIView;
#endif
    return mView->attached(ptr, type) == kResultTrue;
  }
  return false;
}

//------------------------------------------------------------------------
void BaseEditorWrapper::_close() {
  if (mView) {
    mView->setFrame(nullptr);
    mView->removed();
    mView = nullptr;
  }
}

//------------------------------------------------------------------------
bool BaseEditorWrapper::_setKnobMode(Vst::KnobMode val) {
  bool result = false;
  FUnknownPtr<IEditController2> editController2(mController);
  if (editController2)
    result = editController2->setKnobMode(val) == kResultTrue;

  return result;
}

//------------------------------------------------------------------------
// MemoryStream with attributes to add information "preset or project"
//------------------------------------------------------------------------
class VstPresetStream : public MemoryStream, Vst::IStreamAttributes {
public:
  VstPresetStream() {}
  VstPresetStream(void *memory, TSize memorySize)
      : MemoryStream(memory, memorySize) {}

  //---from Vst::IStreamAttributes-----
  tresult PLUGIN_API getFileName(String128 name) SMTG_OVERRIDE {
    return kNotImplemented;
  }
  IAttributeList *PLUGIN_API getAttributes() SMTG_OVERRIDE { return &attrList; }

  //------------------------------------------------------------------------
  DELEGATE_REFCOUNT(MemoryStream)
  tresult PLUGIN_API queryInterface(const TUID iid, void **obj) SMTG_OVERRIDE {
    QUERY_INTERFACE(iid, obj, IStreamAttributes::iid, IStreamAttributes)
    return MemoryStream::queryInterface(iid, obj);
  }

protected:
  HostAttributeList attrList;
};

//------------------------------------------------------------------------
// BaseWrapper
//------------------------------------------------------------------------
BaseWrapper::BaseWrapper(SVST3Config &config) {
  mPlugInterfaceSupport = owned(NEW PlugInterfaceSupport);

  //	mProcessor = owned (config.processor);
  //      mComponent = owned (config.component);
  //	mController = owned (config.controller);
  mFactory = config.factory; // share it
  mVst3EffectClassID = config.vst3ComponentID;

  memset(mName, 0, sizeof(mName));
  memset(mVendor, 0, sizeof(mVendor));
  memset(mSubCategories, 0, sizeof(mSubCategories));

  memset(&mProcessContext, 0, sizeof(ProcessContext));
  mProcessContext.sampleRate = 44100;
  mProcessContext.tempo = 120;

  for (int32 b = 0; b < kMaxMidiMappingBusses; b++) {
    for (int32 i = 0; i < 16; i++) {
      mMidiCCMapping[b][i] = nullptr;
    }
  }
  for (int32 i = 0; i < kMaxProgramChangeParameters; i++) {
    mProgramChangeParameterIDs[i] = kNoParamId;
    mProgramChangeParameterIdxs[i] = -1;
  }
}

//------------------------------------------------------------------------
BaseWrapper::~BaseWrapper() {
  mTimer = nullptr;

  mProcessData.unprepare();

  //---Disconnect components
  if (mComponentsConnected) {
    FUnknownPtr<IConnectionPoint> cp1(mComponent);
    FUnknownPtr<IConnectionPoint> cp2(mController);
    if (cp1 && cp2) {
      cp1->disconnect(cp2);
      cp2->disconnect(cp1);
    }
  }

  //---Terminate Controller Component
  if (mController) {
    mController->setComponentHandler(nullptr);
    if (mControllerInitialized)
      mController->terminate();
    mControllerInitialized = false;
  }

  //---Terminate Processor Component
  if (mComponent && mComponentInitialized) {
    mComponent->terminate();
    mComponentInitialized = false;
  }

  mInputEvents = nullptr;
  mOutputEvents = nullptr;
  mUnitInfo = nullptr;
  mMidiMapping = nullptr;

  if (mMidiCCMapping[0])
    for (int32 b = 0; b < kMaxMidiMappingBusses; b++)
      for (int32 i = 0; i < 16; i++)
        delete mMidiCCMapping[b][i];

  mEditor = nullptr;
  mController = nullptr;
  mProcessor = nullptr;
  mComponent = nullptr;
  mFactory = nullptr;
  mPlugInterfaceSupport = nullptr;

  DeinitModule();
}

//------------------------------------------------------------------------
bool BaseWrapper::init() {
  FUnknown *gStandardPluginContext = 0;

  tresult result = mFactory->createInstance(mVst3EffectClassID, IComponent::iid,
                                            (void **)&mComponent);
  if (!mComponent || (result != kResultOk))
    return false;

  if (mComponent->initialize((IHostApplication *)this) != kResultTrue)
    mComponent->initialize(gStandardPluginContext);
  mComponent->queryInterface(IAudioProcessor::iid, (void **)&mProcessor);
  if (!mProcessor)
    return false;
  //       if(config.processor->canProcessSampleSize (kSample32) != kResultTrue)
  //       return nullptr;
  if (mComponent->queryInterface(IEditController::iid, (void **)&mController) !=
      kResultTrue) {
    TUID controllerCID;
    if (mComponent->getControllerClassId(controllerCID) == kResultTrue) {
      result = mFactory->createInstance(controllerCID, IEditController::iid,
                                        (void **)&mController);
      if (mController->initialize((IHostApplication *)this) != kResultTrue)
        mController->initialize(gStandardPluginContext);
    }
  }

  if (mController) {
    mController->queryInterface(IUnitInfo::iid, (void **)&mUnitInfo);
    mController->queryInterface(IMidiMapping::iid, (void **)&mMidiMapping);

    mComponentInitialized = true;
    mControllerInitialized = true;

    mController->setComponentHandler(this);

    //---connect the 2 components
    FUnknownPtr<IConnectionPoint> cp1(mComponent);
    FUnknownPtr<IConnectionPoint> cp2(mController);
    if (cp1 && cp2) {
      cp1->connect(cp2);
      cp2->connect(cp1);

      mComponentsConnected = true;
    }

    //---inform the component "controller" with the component "processor" state
    MemoryStream stream;
    if (mComponent->getState(&stream) == kResultTrue) {
      stream.seek(0, IBStream::kIBSeekSet, nullptr);
      mController->setComponentState(&stream);
    }
  }

  // Wrapper ----------------------------------------------
  if (mProcessor->canProcessSampleSize(kSample64) == kResultTrue) {
    _canDoubleReplacing(true); // supports double precision processing

    // we use the 64 as default only if 32 bit not supported
    if (mProcessor->canProcessSampleSize(kSample32) != kResultTrue)
      mVst3SampleSize = kSample64;
    else
      mVst3SampleSize = kSample32;
  }

  // latency -------------------------------
  _setInitialDelay(mProcessor->getLatencySamples());

  if (mProcessor->getTailSamples() == kNoTail)
    _noTail(true);

  setupProcessing(); // initialize vst3 component processing parameters

  // parameters
  setupParameters();

  // setup inputs and outputs
  setupBuses();

  // find out programs of root unit --------------------------
  mNumPrograms = 0;
  if (mUnitInfo) {
    int32 programListCount = mUnitInfo->getProgramListCount();
    if (programListCount > 0) {
      ProgramListID rootUnitProgramListId = kNoProgramListId;
      for (int32 i = 0; i < mUnitInfo->getUnitCount(); i++) {
        UnitInfo unit = {0};
        if (mUnitInfo->getUnitInfo(i, unit) == kResultTrue) {
          if (unit.id == kRootUnitId) {
            rootUnitProgramListId = unit.programListId;
            break;
          }
        }
      }

      if (rootUnitProgramListId != kNoProgramListId) {
        for (int32 i = 0; i < programListCount; i++) {
          ProgramListInfo progList = {0};
          if (mUnitInfo->getProgramListInfo(i, progList) == kResultTrue) {
            if (progList.id == rootUnitProgramListId) {
              mNumPrograms = progList.programCount;
              break;
            }
          }
        }
      }
    }
  }

  if (mTimer == nullptr)
    mTimer = owned(Timer::create(this, 50));

  initMidiCtrlerAssignment();

  return true;
}

//------------------------------------------------------------------------
void BaseWrapper::_suspend() {
  _stopProcess();

  if (mComponent)
    mComponent->setActive(false);
  mActive = false;
}

//------------------------------------------------------------------------
void BaseWrapper::_resume() {
  mChunk.setSize(0);

  if (mComponent)
    mComponent->setActive(true);
  mActive = true;
}

//------------------------------------------------------------------------
void BaseWrapper::_startProcess() {
  if (mProcessor && mProcessing == false) {
    mProcessing = true;
    mProcessor->setProcessing(true);
  }
}

//------------------------------------------------------------------------
void BaseWrapper::_stopProcess() {
  if (mProcessor && mProcessing) {
    mProcessor->setProcessing(false);
    mProcessing = false;
  }
}

//------------------------------------------------------------------------
void BaseWrapper::_setEditor(BaseEditorWrapper *editor) {
  mEditor = owned(editor);
}

//------------------------------------------------------------------------
bool BaseWrapper::_setBlockSize(int32 newBlockSize) {
  if (mProcessing)
    return false;

  if (mBlockSize != newBlockSize) {
    mBlockSize = newBlockSize;
    setupProcessing();
    return true;
  }
  return false;
}

//------------------------------------------------------------------------
bool BaseWrapper::setupProcessing(int32 processModeOverwrite) {
  if (!mProcessor)
    return false;

  ProcessSetup setup;
  if (processModeOverwrite >= 0)
    setup.processMode = processModeOverwrite;
  else
    setup.processMode = mVst3processMode;
  setup.maxSamplesPerBlock = mBlockSize;
  setup.sampleRate = mSampleRate;
  setup.symbolicSampleSize = mVst3SampleSize;

  return mProcessor->setupProcessing(setup) == kResultTrue;
}

//------------------------------------------------------------------------
bool BaseWrapper::getEditorSize(int32 &width, int32 &height) const {
  if (auto *editor = getEditor()) {
    ViewRect rect;
    if (editor->getRect(rect)) {
      width = rect.right - rect.left;
      height = rect.bottom - rect.top;
      return true;
    }
  }
  return false;
}

//------------------------------------------------------------------------
float BaseWrapper::_getParameter(int32 index) const {
  if (!mController)
    return 0.f;

  if (index < (int32)mParameterMap.size()) {
    ParamID id = mParameterMap.at(index).vst3ID;
    return (float)mController->getParamNormalized(id);
  }

  return 0.f;
}

//------------------------------------------------------------------------
void BaseWrapper::addParameterChange(ParamID id, ParamValue value,
                                     int32 sampleOffset) {
  mGuiTransfer.addChange(id, value, sampleOffset);
  mInputTransfer.addChange(id, value, sampleOffset);
}

//------------------------------------------------------------------------
/*!	Usually VST 2 hosts call setParameter (...) and getParameterDisplay
(...) synchronously. In setParameter (...) param changes get queued
(guiTransfer) and transfered in idle (::onTimer). The ::onTimer call almost
always comes AFTER getParameterDisplay (...) and therefore returns an old value.
To avoid sending back old values, getLastParamChange (...) returns the latest
value from the guiTransfer queue. */
//------------------------------------------------------------------------
bool BaseWrapper::getLastParamChange(ParamID id, ParamValue &value) {
  ParameterChanges changes;
  mGuiTransfer.transferChangesTo(changes);
  for (int32 i = 0; i < changes.getParameterCount(); ++i) {
    IParamValueQueue *queue = changes.getParameterData(i);
    if (queue) {
      if (queue->getParameterId() == id) {
        int32 points = queue->getPointCount();
        if (points > 0) {
          mGuiTransfer.transferChangesFrom(changes);
          int32 sampleOffset = 0;
          return queue->getPoint(points - 1, sampleOffset, value) ==
                 kResultTrue;
        }
      }
    }
  }

  mGuiTransfer.transferChangesFrom(changes);
  return false;
}

//------------------------------------------------------------------------
void BaseWrapper::getUnitPath(UnitID unitID, String &path) const {
  if (!mUnitInfo)
    return;

  //! Build the unit path up to the root unit (e.g. "Modulators.LFO 1.".
  //! Separator is a ".")
  for (int32 unitIndex = 0; unitIndex < mUnitInfo->getUnitCount();
       ++unitIndex) {
    UnitInfo info = {0};
    mUnitInfo->getUnitInfo(unitIndex, info);
    if (info.id == unitID) {
      String unitName(info.name);
      unitName.append(".");
      path.insertAt(0, unitName);
      if (info.parentUnitId != kRootUnitId)
        getUnitPath(info.parentUnitId, path);

      break;
    }
  }
}
//------------------------------------------------------------------------
int32 BaseWrapper::_getChunk(void **data, bool isPreset) {
  // Host stores Plug-in state. Returns the size in bytes of the chunk (Plug-in
  // allocates the data array)
  MemoryStream componentStream;
  if (mComponent && mComponent->getState(&componentStream) != kResultTrue)
    componentStream.setSize(0);

  MemoryStream controllerStream;
  if (mController && mController->getState(&controllerStream) != kResultTrue)
    controllerStream.setSize(0);

  if (componentStream.getSize() + controllerStream.getSize() == 0)
    return 0;

  mChunk.setSize(0);
  IBStreamer acc(&mChunk, kLittleEndian);

  acc.writeInt64(componentStream.getSize());
  acc.writeInt64(controllerStream.getSize());

  acc.writeRaw(componentStream.getData(), (int32)componentStream.getSize());
  acc.writeRaw(controllerStream.getData(), (int32)controllerStream.getSize());

  int32 chunkSize = (int32)mChunk.getSize();
  *data = mChunk.getData();
  return chunkSize;
}

//------------------------------------------------------------------------
int32 BaseWrapper::_setChunk(void *data, int32 byteSize, bool isPreset) {
  if (!mComponent)
    return 0;

  // throw away all previously queued parameter changes, they are obsolete
  mGuiTransfer.removeChanges();
  mInputTransfer.removeChanges();

  MemoryStream chunk(data, byteSize);
  IBStreamer acc(&chunk, kLittleEndian);

  int64 componentDataSize = 0;
  int64 controllerDataSize = 0;

  acc.readInt64(componentDataSize);
  acc.readInt64(controllerDataSize);

  VstPresetStream componentStream(((char *)data) + acc.tell(),
                                  componentDataSize);
  VstPresetStream controllerStream(
      ((char *)data) + acc.tell() + componentDataSize, controllerDataSize);

  mComponent->setState(&componentStream);
  componentStream.seek(0, IBStream::kIBSeekSet, nullptr);

  if (mController) {
    if (!isPreset) {
      if (Vst::IAttributeList *attr = componentStream.getAttributes())
        attr->setString(Vst::PresetAttributes::kStateType,
                        String(Vst::StateType::kProject));
      if (Vst::IAttributeList *attr = controllerStream.getAttributes())
        attr->setString(Vst::PresetAttributes::kStateType,
                        String(Vst::StateType::kProject));
    }
    mController->setComponentState(&componentStream);
    mController->setState(&controllerStream);
  }

  return 0;
}

//------------------------------------------------------------------------
bool BaseWrapper::_setBypass(bool onOff) {
  if (mBypassParameterID != kNoParamId) {
    addParameterChange(mBypassParameterID, onOff ? 1.0 : 0.0, 0);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool BaseWrapper::getProgramListAndUnit(int32 midiChannel, UnitID &unitId,
                                        ProgramListID &programListId) {
  programListId = kNoProgramListId;
  unitId = -1;
  if (!mUnitInfo)
    return false;

  // use the first input event bus (VST 2 has only 1 bus for event)
  if (mUnitInfo->getUnitByBus(kEvent, kInput, 0, midiChannel, unitId) ==
      kResultTrue) {
    for (int32 i = 0, unitCount = mUnitInfo->getUnitCount(); i < unitCount;
         i++) {
      UnitInfo unitInfoStruct = {0};
      if (mUnitInfo->getUnitInfo(i, unitInfoStruct) == kResultTrue) {
        if (unitId == unitInfoStruct.id) {
          programListId = unitInfoStruct.programListId;
          return programListId != kNoProgramListId;
        }
      }
    }
  }

  return false;
}

//-----------------------------------------------------------------------------
bool BaseWrapper::getProgramListInfoByProgramListID(ProgramListID programListId,
                                                    ProgramListInfo &info) {
  if (mUnitInfo) {
    int32 programListCount = mUnitInfo->getProgramListCount();
    for (int32 i = 0; i < programListCount; i++) {
      memset(&info, 0, sizeof(ProgramListInfo));
      if (mUnitInfo->getProgramListInfo(i, info) == kResultTrue) {
        if (info.id == programListId) {
          return true;
        }
      }
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void BaseWrapper::setVendorName(char *name) {
  memcpy(mVendor, name, sizeof(mVendor));
}

//-----------------------------------------------------------------------------
void BaseWrapper::setEffectName(char *effectName) {
  memcpy(mName, effectName, sizeof(mName));
}

//-----------------------------------------------------------------------------
void BaseWrapper::setEffectVersion(char *version) {
  if (!version)
    mVersion = 0;
  else {
    int32 major = 1;
    int32 minor = 0;
    int32 subminor = 0;
    int32 subsubminor = 0;
    int32 ret =
        sscanf(version, "%d.%d.%d.%d", &major, &minor, &subminor, &subsubminor);
    mVersion = (major & 0xff) << 24;
    if (ret > 3)
      mVersion += (subsubminor & 0xff);
    if (ret > 2)
      mVersion += (subminor & 0xff) << 8;
    if (ret > 1)
      mVersion += (minor & 0xff) << 16;
  }
}

//-----------------------------------------------------------------------------
void BaseWrapper::setSubCategories(char *string) {
  memcpy(mSubCategories, string, sizeof(mSubCategories));
}

//-----------------------------------------------------------------------------
void BaseWrapper::setupBuses() {
  if (!mComponent)
    return;

  mProcessData.prepare(*mComponent, 0, mVst3SampleSize);

  _setNumInputs(countMainBusChannels(kInput, mMainAudioInputBuses));
  _setNumOutputs(countMainBusChannels(kOutput, mMainAudioOutputBuses));

  mHasEventInputBuses = mComponent->getBusCount(kEvent, kInput) > 0;
  mHasEventOutputBuses = mComponent->getBusCount(kEvent, kOutput) > 0;

  if (mHasEventInputBuses) {
    if (mInputEvents == nullptr)
      mInputEvents = owned(new EventList(kMaxEvents));
  } else
    mInputEvents = nullptr;

  if (mHasEventOutputBuses) {
    if (mOutputEvents == nullptr)
      mOutputEvents = owned(new EventList(kMaxEvents));
  } else
    mOutputEvents = nullptr;
}

//-----------------------------------------------------------------------------
void BaseWrapper::setupParameters() {
  mParameterMap.clear();
  mParamIndexMap.clear();
  mBypassParameterID = mProgramParameterID = kNoParamId;
  mProgramParameterIdx = -1;

  std::vector<ParameterInfo> programParameterInfos;
  std::vector<int32> programParameterIdxs;

  int32 paramCount = mController ? mController->getParameterCount() : 0;
  int32 numParamID = 0;
  for (int32 i = 0; i < paramCount; i++) {
    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(i, paramInfo) == kResultTrue) {
      //--- ------------------------------------------
      if ((paramInfo.flags & ParameterInfo::kIsBypass) != 0) {
        if (mBypassParameterID == kNoParamId)
          mBypassParameterID = paramInfo.id;
        if (mUseExportedBypass) {
          ParamMapEntry entry = {paramInfo.id, i};
          mParameterMap.push_back(entry);
          mParamIndexMap[paramInfo.id] = mUseIncIndex ? numParamID : i;
          numParamID++;
        }
      }
      //--- ------------------------------------------
      else if ((paramInfo.flags & ParameterInfo::kIsProgramChange) != 0) {
        programParameterInfos.push_back(paramInfo);
        programParameterIdxs.push_back(i);
        if (paramInfo.unitId == kRootUnitId) {
          if (mProgramParameterID == kNoParamId) {
            mProgramParameterID = paramInfo.id;
            mProgramParameterIdx = i;
          }
        }

        if (gExportProgramChangeParameters == true) {
          ParamMapEntry entry = {paramInfo.id, i};
          mParameterMap.push_back(entry);
          mParamIndexMap[paramInfo.id] = mUseIncIndex ? numParamID : i;
          numParamID++;
        }
      }
      //--- ------------------------------------------
      // do not export read only parameters
      else if ((paramInfo.flags & ParameterInfo::kIsReadOnly) == 0) {
        ParamMapEntry entry = {paramInfo.id, i};
        mParameterMap.push_back(entry);
        mParamIndexMap[paramInfo.id] = mUseIncIndex ? numParamID : i;
        numParamID++;
      }
    }
  }

  mNumParams = (int32)mParameterMap.size();

  mInputTransfer.setMaxParameters(paramCount);
  mOutputTransfer.setMaxParameters(paramCount);
  mGuiTransfer.setMaxParameters(paramCount);
  mInputChanges.setMaxParameters(paramCount);
  mOutputChanges.setMaxParameters(paramCount);

  for (int32 midiChannel = 0; midiChannel < kMaxProgramChangeParameters;
       midiChannel++) {
    mProgramChangeParameterIDs[midiChannel] = kNoParamId;
    mProgramChangeParameterIdxs[midiChannel] = -1;

    UnitID unitId;
    ProgramListID programListId;
    if (getProgramListAndUnit(midiChannel, unitId, programListId)) {
      for (int32 i = 0; i < (int32)programParameterInfos.size(); i++) {
        const ParameterInfo &paramInfo = programParameterInfos.at(i);
        if (paramInfo.unitId == unitId) {
          mProgramChangeParameterIDs[midiChannel] = paramInfo.id;
          mProgramChangeParameterIdxs[midiChannel] = programParameterIdxs.at(i);
          break;
        }
      }
    }
  }
}

//-----------------------------------------------------------------------------
void BaseWrapper::initMidiCtrlerAssignment() {
  if (!mMidiMapping || !mComponent)
    return;

  int32 busses = Min<int32>(mComponent->getBusCount(kEvent, kInput),
                            kMaxMidiMappingBusses);

  if (!mMidiCCMapping[0][0]) {
    for (int32 b = 0; b < busses; b++)
      for (int32 i = 0; i < 16; i++)
        mMidiCCMapping[b][i] = NEW int32[Vst::kCountCtrlNumber];
  }

  ParamID paramID;
  for (int32 b = 0; b < busses; b++) {
    for (int16 ch = 0; ch < 16; ch++) {
      for (int32 i = 0; i < Vst::kCountCtrlNumber; i++) {
        paramID = kNoParamId;
        if (mMidiMapping->getMidiControllerAssignment(b, ch, (CtrlNumber)i,
                                                      paramID) == kResultTrue) {
          // TODO check if tag is associated to a parameter
          mMidiCCMapping[b][ch][i] = paramID;
        } else
          mMidiCCMapping[b][ch][i] = kNoParamId;
      }
    }
  }
}

//-----------------------------------------------------------------------------
void BaseWrapper::_setSampleRate(float newSamplerate) {
  if (mProcessing)
    return;

  if (newSamplerate != mSampleRate) {
    mSampleRate = newSamplerate;
    setupProcessing();
  }
}

//-----------------------------------------------------------------------------
int32 BaseWrapper::countMainBusChannels(BusDirection dir,
                                        uint64 &mainBusBitset) {
  int32 result = 0;
  mainBusBitset = 0;

  int32 busCount = mComponent->getBusCount(kAudio, dir);
  for (int32 i = 0; i < busCount; i++) {
    BusInfo busInfo = {0};
    if (mComponent->getBusInfo(kAudio, dir, i, busInfo) == kResultTrue) {
      if (busInfo.busType == kMain || busInfo.busType == kAux) {
        result += busInfo.channelCount;
        mainBusBitset |= (uint64(1) << i);

        mComponent->activateBus(kAudio, dir, i, true);
      } else if (busInfo.flags & BusInfo::kDefaultActive) {
        mComponent->activateBus(kAudio, dir, i, false);
      }
    }
  }
  return result;
}

//-----------------------------------------------------------------------------
void BaseWrapper::processMidiEvent(Event &toAdd, char *midiData, bool isLive,
                                   int32 noteLength, float noteOffVelocity,
                                   float detune) {
  uint8 status = midiData[0] & kStatusMask;
  uint8 channel = midiData[0] & kChannelMask;

  // not allowed
  if (channel >= 16)
    return;

  if (isLive)
    toAdd.flags |= Event::kIsLive;

  //--- -----------------------------
  switch (status) {
  case kNoteOn:
  case kNoteOff: {
    if (status == kNoteOff || midiData[2] == 0) // note off
    {
      toAdd.type = Event::kNoteOffEvent;
      toAdd.noteOff.channel = channel;
      toAdd.noteOff.pitch = midiData[1];
      toAdd.noteOff.velocity = noteOffVelocity;
      toAdd.noteOff.noteId = (channel << 8 | midiData[1]);
    } else if (status == kNoteOn) // note on
    {
      toAdd.type = Event::kNoteOnEvent;
      toAdd.noteOn.channel = channel;
      toAdd.noteOn.pitch = midiData[1];
      toAdd.noteOn.tuning = detune;
      toAdd.noteOn.velocity = (float)midiData[2] * kMidiScaler;
      toAdd.noteOn.length = noteLength;
      toAdd.noteOn.noteId = (channel << 8 | midiData[1]);
    }
    mInputEvents->addEvent(toAdd);
  } break;
  //--- -----------------------------
  case kPolyPressure: {
    toAdd.type = Vst::Event::kPolyPressureEvent;
    toAdd.polyPressure.channel = channel;
    toAdd.polyPressure.pitch = midiData[1] & kDataMask;
    toAdd.polyPressure.pressure = (midiData[2] & kDataMask) * kMidiScaler;
    toAdd.polyPressure.noteId = (channel << 8 | midiData[1]);

    mInputEvents->addEvent(toAdd);
  } break;
  //--- -----------------------------
  case kController: {
    if (toAdd.busIndex < kMaxMidiMappingBusses &&
        mMidiCCMapping[toAdd.busIndex][channel]) {
      ParamID paramID = mMidiCCMapping[toAdd.busIndex][channel]
                                      [static_cast<size_t>(midiData[1])];
      if (paramID != kNoParamId) {
        ParamValue value = (double)midiData[2] * kMidiScaler;

        int32 index = 0;
        IParamValueQueue *queue =
            mInputChanges.addParameterData(paramID, index);
        if (queue) {
          queue->addPoint(toAdd.sampleOffset, value, index);
        }
        mGuiTransfer.addChange(paramID, value, toAdd.sampleOffset);
      }
    }
  } break;
  //--- -----------------------------
  case kPitchBendStatus: {
    if (toAdd.busIndex < kMaxMidiMappingBusses &&
        mMidiCCMapping[toAdd.busIndex][channel]) {
      ParamID paramID =
          mMidiCCMapping[toAdd.busIndex][channel][Vst::kPitchBend];
      if (paramID != kNoParamId) {
        const double kPitchWheelScaler = 1. / (double)0x3FFF;

        const int32 ctrl = (midiData[1] & kDataMask) | (midiData[2] & kDataMask)
                                                           << 7;
        ParamValue value = kPitchWheelScaler * (double)ctrl;

        int32 index = 0;
        IParamValueQueue *queue =
            mInputChanges.addParameterData(paramID, index);
        if (queue) {
          queue->addPoint(toAdd.sampleOffset, value, index);
        }
        mGuiTransfer.addChange(paramID, value, toAdd.sampleOffset);
      }
    }
  } break;
  //--- -----------------------------
  case kAfterTouchStatus: {
    if (toAdd.busIndex < kMaxMidiMappingBusses &&
        mMidiCCMapping[toAdd.busIndex][channel]) {
      ParamID paramID =
          mMidiCCMapping[toAdd.busIndex][channel][Vst::kAfterTouch];
      if (paramID != kNoParamId) {
        ParamValue value = (ParamValue)(midiData[1] & kDataMask) * kMidiScaler;

        int32 index = 0;
        IParamValueQueue *queue =
            mInputChanges.addParameterData(paramID, index);
        if (queue) {
          queue->addPoint(toAdd.sampleOffset, value, index);
        }
        mGuiTransfer.addChange(paramID, value, toAdd.sampleOffset);
      }
    }
  } break;
  //--- -----------------------------
  case kProgramChangeStatus: {
    if (mProgramChangeParameterIDs[channel] != kNoParamId &&
        mProgramChangeParameterIdxs[channel] != -1) {
      ParameterInfo paramInfo = {0};
      if (mController->getParameterInfo(mProgramChangeParameterIdxs[channel],
                                        paramInfo) == kResultTrue) {
        int32 program = midiData[1];
        if (paramInfo.stepCount > 0 && program <= paramInfo.stepCount) {
          ParamValue normalized =
              (ParamValue)program / (ParamValue)paramInfo.stepCount;
          addParameterChange(mProgramChangeParameterIDs[channel], normalized,
                             toAdd.sampleOffset);
        }
      }
    }
  } break;
  }
}

//-----------------------------------------------------------------------------
template <class T>
inline void BaseWrapper::setProcessingBuffers(T **inputs, T **outputs) {
  // set processing buffers
  int32 sourceIndex = 0;
  for (int32 i = 0; i < mProcessData.numInputs; i++) {
    AudioBusBuffers &buffers = mProcessData.inputs[i];
    if (mMainAudioInputBuses & (uint64(1) << i)) {
      for (int32 j = 0; j < buffers.numChannels; j++) {
        buffers.channelBuffers32[j] = (Sample32 *)inputs[sourceIndex++];
      }
    } else
      buffers.silenceFlags = HostProcessData::kAllChannelsSilent;
  }

  sourceIndex = 0;
  for (int32 i = 0; i < mProcessData.numOutputs; i++) {
    AudioBusBuffers &buffers = mProcessData.outputs[i];
    buffers.silenceFlags = 0;
    if (mMainAudioOutputBuses & (uint64(1) << i)) {
      for (int32 j = 0; j < buffers.numChannels; j++) {
        buffers.channelBuffers32[j] = (Sample32 *)outputs[sourceIndex++];
      }
    }
  }
}

//-----------------------------------------------------------------------------
inline void BaseWrapper::setEventPPQPositions() {
  if (!mInputEvents)
    return;

  int32 eventCount = mInputEvents->getEventCount();
  if (eventCount > 0 && (mProcessContext.state & ProcessContext::kTempoValid) &&
      (mProcessContext.state & ProcessContext::kProjectTimeMusicValid)) {
    TQuarterNotes projectTimeMusic = mProcessContext.projectTimeMusic;
    double secondsToQuarterNoteScaler = mProcessContext.tempo / 60.0;
    double multiplicator = secondsToQuarterNoteScaler / mSampleRate;

    for (int32 i = 0; i < eventCount; i++) {
      Event *e = mInputEvents->getEventByIndex(i);
      if (e) {
        TQuarterNotes localTimeMusic = e->sampleOffset * multiplicator;
        e->ppqPosition = projectTimeMusic + localTimeMusic;
      }
    }
  }
}

//-----------------------------------------------------------------------------
inline void BaseWrapper::doProcess(int32 sampleFrames) {
  if (!mProcessor)
    return;

  mProcessData.numSamples = sampleFrames;

  if (mProcessing == false)
    _startProcess();

  mProcessData.inputEvents = mInputEvents;
  mProcessData.outputEvents = mOutputEvents;

  setupProcessTimeInfo();
  setEventPPQPositions();

  mInputTransfer.transferChangesTo(mInputChanges);

  mProcessData.inputParameterChanges = &mInputChanges;
  mProcessData.outputParameterChanges = &mOutputChanges;
  mOutputChanges.clearQueue();

  // VST 3 process call
  mProcessor->process(mProcessData);

  processOutputParametersChanges();

  mOutputTransfer.transferChangesFrom(mOutputChanges);
  processOutputEvents();

  // clear input parameters and events
  mInputChanges.clearQueue();
  if (mInputEvents)
    mInputEvents->clear();
}

//-----------------------------------------------------------------------------
void BaseWrapper::_processReplacing(float **inputs, float **outputs,
                                    int32 sampleFrames) {
  if (mProcessData.symbolicSampleSize != kSample32)
    return;

  setProcessingBuffers<float>(inputs, outputs);
  doProcess(sampleFrames);
}

//-----------------------------------------------------------------------------
void BaseWrapper::_processDoubleReplacing(double **inputs, double **outputs,
                                          int32 sampleFrames) {
  if (mProcessData.symbolicSampleSize != kSample64)
    return;

  setProcessingBuffers<double>(inputs, outputs);
  doProcess(sampleFrames);
}

//-----------------------------------------------------------------------------
void BaseWrapper::onTimer(Timer *) {
  if (!mController)
    return;

  ParamID id;
  ParamValue value;
  int32 sampleOffset;

  while (mOutputTransfer.getNextChange(id, value, sampleOffset)) {
    mController->setParamNormalized(id, value);
  }
  while (mGuiTransfer.getNextChange(id, value, sampleOffset)) {
    mController->setParamNormalized(id, value);
  }
}

// FUnknown
//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::queryInterface(const char *iid, void **obj) {
  QUERY_INTERFACE(iid, obj, FUnknown::iid, Vst::IHostApplication)
  QUERY_INTERFACE(iid, obj, Vst::IHostApplication::iid, Vst::IHostApplication)
  QUERY_INTERFACE(iid, obj, Vst::IComponentHandler::iid, Vst::IComponentHandler)
  QUERY_INTERFACE(iid, obj, Vst::IUnitHandler::iid, Vst::IUnitHandler)

  if (mPlugInterfaceSupport &&
      mPlugInterfaceSupport->queryInterface(iid, obj) == kResultTrue)
    return ::Steinberg::kResultOk;

  *obj = nullptr;
  return kNoInterface;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::restartComponent(int32 flags) {
  tresult result = kResultFalse;

  //--- ----------------------
  if (flags & kIoChanged) {
    setupBuses();
    _ioChanged();
    result = kResultTrue;
  }

  //--- ----------------------
  if ((flags & kParamValuesChanged) || (flags & kParamTitlesChanged)) {
    _updateDisplay();
    result = kResultTrue;
  }

  //--- ----------------------
  if (flags & kLatencyChanged) {
    if (mProcessor)
      _setInitialDelay(mProcessor->getLatencySamples());

    _ioChanged();
    result = kResultTrue;
  }

  //--- ----------------------
  if (flags & kMidiCCAssignmentChanged) {
    initMidiCtrlerAssignment();
    result = kResultTrue;
  }

  // kReloadComponent is Not supported

  return result;
}

//-----------------------------------------------------------------------------
// IHostApplication
//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::createInstance(TUID cid, TUID iid, void **obj) {
  FUID classID(FUID::fromTUID(cid));
  FUID interfaceID(FUID::fromTUID(iid));
  if (classID == IMessage::iid && interfaceID == IMessage::iid) {
    *obj = new HostMessage;
    return kResultTrue;
  } else if (classID == IAttributeList::iid &&
             interfaceID == IAttributeList::iid) {
    *obj = new HostAttributeList;
    return kResultTrue;
  }
  *obj = nullptr;
  return kResultFalse;
}

//-----------------------------------------------------------------------------
// IUnitHandler
//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::notifyUnitSelection(UnitID unitId) {
  return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API BaseWrapper::notifyProgramListChange(ProgramListID listId,
                                                        int32 programIndex) {
  // TODO -> redirect to hasMidiProgramsChanged somehow...
  return kResultTrue;
}

//-----------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

//-----------------------------------------------------------------------------

/// \endcond

//extern bool DeinitModule(); //! Called in Vst2Wrapper destructor

//------------------------------------------------------------------------
// some Defines
//------------------------------------------------------------------------
// should be kVstMaxParamStrLen if we want to respect the VST 2 specification!!!
#define kVstExtMaxParamStrLen 32

//------------------------------------------------------------------------
namespace Steinberg {
namespace Vst {

//! The parameter's name contains the unit path (e.g.
//! "Modulators.LFO 1.frequency")
// bool vst2WrapperFullParameterPath = true;
bool vst2WrapperFullParameterPath = false;
/*
//------------------------------------------------------------------------
// Vst2EditorWrapper Declaration
//------------------------------------------------------------------------
class Vst2EditorWrapper : public BaseEditorWrapper {
public:
  //------------------------------------------------------------------------
  Vst2EditorWrapper(IEditController *controller,
                    audioMasterCallback audioMaster);

  //--- from BaseEditorWrapper ---------------------
  void _close();

  //--- from AEffEditor-------------------
  bool getRect(ERect **rect);
  bool open(void *ptr);
  void close() { _close(); }
  bool setKnobMode(VstInt32 val) {
    return BaseEditorWrapper::_setKnobMode(static_cast<Vst::KnobMode>(val));
  }

  //--- IPlugFrame ----------------------------
  tresult PLUGIN_API resizeView(IPlugView *view, ViewRect *newSize);

  audioMasterCallback audioMaster3;

  //------------------------------------------------------------------------
protected:
  ERect mERect;
};
*/
//------------------------------------------------------------------------
bool areSizeEquals(const ViewRect &r1, const ViewRect &r2) {
  if (r1.getHeight() != r2.getHeight())
    return false;
  if (r1.getWidth() != r2.getWidth())
    return false;
  return true;
}

// #pragma GCC optimize("O0")

//------------------------------------------------------------------------
// Vst2EditorWrapper Implementation
//------------------------------------------------------------------------
Vst2EditorWrapper::Vst2EditorWrapper(IEditController *controller,
                                     audioMasterCallback audioMaster)
    : BaseEditorWrapper(controller) {
  audioMaster3 = audioMaster;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Vst2EditorWrapper::resizeView(IPlugView *view,
                                                 ViewRect *newSize) {
  AEffect plugin;

  if (audioMaster3)
  {
  BaseEditorWrapper::resizeView(view, newSize);
  audioMaster3(&plugin, audioMasterSizeWindow, newSize->getWidth(), newSize->getHeight(), 0, 0);
  return true;
  }
  return false;
}

//------------------------------------------------------------------------
bool Vst2EditorWrapper::getRect(ERect **rect) {
  ViewRect size;
  if (BaseEditorWrapper::getRect(size)) {
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
bool Vst2EditorWrapper::open(void *ptr) {
  return BaseEditorWrapper::_open(ptr);
}

//------------------------------------------------------------------------
void Vst2EditorWrapper::_close() { BaseEditorWrapper::_close(); }

// #pragma GCC optimize("O2")

//------------------------------------------------------------------------
// Vst2MidiEventQueue Declaration
//------------------------------------------------------------------------
class Vst2MidiEventQueue {
public:
  //------------------------------------------------------------------------
  Vst2MidiEventQueue(int32 maxEventCount);
  ~Vst2MidiEventQueue();

  bool isEmpty() const { return eventList->numEvents == 0; }
  bool add(const VstMidiEvent &e);
  void flush();

  operator VstEvents *() { return eventList; }
  //------------------------------------------------------------------------
protected:
  VstEvents *eventList;
  int32 maxEventCount;
};

//------------------------------------------------------------------------
// Vst2MidiEventQueue Implementation
//------------------------------------------------------------------------
Vst2MidiEventQueue::Vst2MidiEventQueue(int32 _maxEventCount)
    : maxEventCount(_maxEventCount) {
  eventList = (VstEvents *)new char[sizeof(VstEvents) +
                                    (maxEventCount - 2) * sizeof(VstEvent *)];
  eventList->numEvents = 0;
  eventList->reserved = 0;

  int32 eventSize = sizeof(VstMidiEvent);

  for (int32 i = 0; i < maxEventCount; i++) {
    char *eventBuffer = new char[eventSize];
    memset(eventBuffer, 0, eventSize);
    eventList->events[i] = (VstEvent *)eventBuffer;
  }
}

//------------------------------------------------------------------------
Vst2MidiEventQueue::~Vst2MidiEventQueue() {
  for (int32 i = 0; i < maxEventCount; i++)
    delete[](char *) eventList->events[i];

  delete[](char *) eventList;
}

//------------------------------------------------------------------------
bool Vst2MidiEventQueue::add(const VstMidiEvent &e) {
  if (eventList->numEvents >= maxEventCount)
    return false;

  auto *dst = (VstMidiEvent *)eventList->events[eventList->numEvents++];
  memcpy(dst, &e, sizeof(VstMidiEvent));
  dst->type = kVstMidiType;
  dst->byteSize = sizeof(VstMidiEvent);
  return true;
}

//------------------------------------------------------------------------
void Vst2MidiEventQueue::flush() { eventList->numEvents = 0; }

//------------------------------------------------------------------------
// Vst2Wrapper
//------------------------------------------------------------------------
Vst2Wrapper::Vst2Wrapper(BaseWrapper::SVST3Config &config,
                         audioMasterCallback audioMaster2, VstInt32 vst2ID)
    : BaseWrapper(config), editor(0), curProgram(0), initialdelay(0),
      doublereplacing(false), numinputs(0), numoutputs(0), numparams(0),
      numprograms(0), synth(false) {
  mUseExportedBypass = false;
  mUseIncIndex = true;

  audioMaster = audioMaster2;

  //	setUniqueID (vst2ID);
  //	canProcessReplacing (true); // supports replacing output
  //	programsAreChunks (true);
}

//------------------------------------------------------------------------
Vst2Wrapper::~Vst2Wrapper() {
  //! editor needs to be destroyed BEFORE DeinitModule. Therefore destroy it
  //! here already
  //  instead of AudioEffect destructor
  if (mEditor) {
    mEditor = nullptr;
  }

  delete mVst2OutputEvents;
  mVst2OutputEvents = nullptr;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::init(audioMasterCallback audioMaster) {
  //	if (strstr (mSubCategories, "Instrument"))
  //	synth = true;
  //		isSynth (true);

  bool res = BaseWrapper::init();

  numprograms = mNumPrograms;

  if (mController) {
    if (BaseEditorWrapper::hasEditor(mController)) {
      editor = new Vst2EditorWrapper(mController, audioMaster);
      if (editor)
        _setEditor(editor);
    }
  }
  return res;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_canDoubleReplacing(bool val) {
  //	canDoubleReplacing (val);
  doublereplacing = val;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_setInitialDelay(int32 delay) {
  //	setInitialDelay (delay);
  initialdelay = delay;
}

//------------------------------------------------------------------------
void Vst2Wrapper::_noTail(bool val) {
  //	noTail (val);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setupBuses() {
  BaseWrapper::setupBuses();

  if (mHasEventOutputBuses) {
    if (mVst2OutputEvents == nullptr)
      mVst2OutputEvents = new Vst2MidiEventQueue(kMaxEvents);
  } else {
    if (mVst2OutputEvents) {
      delete mVst2OutputEvents;
      mVst2OutputEvents = nullptr;
    }
  }
}
//------------------------------------------------------------------------
void Vst2Wrapper::setupProcessTimeInfo() {
  VstTimeInfo *vst2timeInfo;

  vst2timeInfo = nullptr;

  if (audioMaster)
    vst2timeInfo =
        (VstTimeInfo *)audioMaster(0, audioMasterGetTime, 0, 0, 0, 0);

  if (vst2timeInfo) {
    const uint32 portableFlags =
        ProcessContext::kPlaying | ProcessContext::kCycleActive |
        ProcessContext::kRecording | ProcessContext::kSystemTimeValid |
        ProcessContext::kProjectTimeMusicValid |
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

    if (mProcessContext.state & ProcessContext::kCycleValid) {
      mProcessContext.cycleStartMusic = vst2timeInfo->cycleStartPos;
      mProcessContext.cycleEndMusic = vst2timeInfo->cycleEndPos;
    } else
      mProcessContext.cycleStartMusic = mProcessContext.cycleEndMusic = 0.0;

    if (mProcessContext.state & ProcessContext::kTempoValid)
      mProcessContext.tempo = vst2timeInfo->tempo;
    else
      mProcessContext.tempo = 120.0;

    if (mProcessContext.state & ProcessContext::kTimeSigValid) {
      mProcessContext.timeSigNumerator = vst2timeInfo->timeSigNumerator;
      mProcessContext.timeSigDenominator = vst2timeInfo->timeSigDenominator;
    } else
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
                            mProcessContext.frameRate.flags =
    FrameRate::kPullDownRate; break; case kVstSmpte30fps: ///< 30 fps
                            mProcessContext.frameRate.framesPerSecond = 30;
                            break;
                    case kVstSmpte2997dfps: ///< 29.97 drop
                            mProcessContext.frameRate.framesPerSecond = 30;
                            mProcessContext.frameRate.flags =
                                FrameRate::kPullDownRate | FrameRate::kDropRate;
                            break;
                    case kVstSmpte30dfps: ///< 30 drop
                            mProcessContext.frameRate.framesPerSecond = 30;
                            mProcessContext.frameRate.flags =
    FrameRate::kDropRate; break; case kVstSmpteFilm16mm: // not a smpte rate
                    case kVstSmpteFilm35mm:
                            mProcessContext.state &=
    ~ProcessContext::kSmpteValid; break; case kVstSmpte239fps: ///< 23.9 fps
                            mProcessContext.frameRate.framesPerSecond = 24;
                            mProcessContext.frameRate.flags =
    FrameRate::kPullDownRate; break; case kVstSmpte249fps: ///< 24.9 fps
                            mProcessContext.frameRate.framesPerSecond = 25;
                            mProcessContext.frameRate.flags =
    FrameRate::kPullDownRate; break; case kVstSmpte599fps: ///< 59.9 fps
                            mProcessContext.frameRate.framesPerSecond = 60;
                            mProcessContext.frameRate.flags =
    FrameRate::kPullDownRate; break; case kVstSmpte60fps: ///< 60 fps
                            mProcessContext.frameRate.framesPerSecond = 60;
                            break;
                    default: mProcessContext.state &=
    ~ProcessContext::kSmpteValid; break;
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
    //		mProcessContext.samplesToNextClock =
    //vst2timeInfo->samplesToNextClock; 	else
    mProcessContext.samplesToNextClock = 0;

    mProcessData.processContext = &mProcessContext;
  } else
    mProcessData.processContext = nullptr;
}

//------------------------------------------------------------------------
void Vst2Wrapper::suspend() { BaseWrapper::_suspend(); }

//------------------------------------------------------------------------
void Vst2Wrapper::resume() { BaseWrapper::_resume(); }

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::startProcess() {
  BaseWrapper::_startProcess();
  return 0;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::stopProcess() {
  BaseWrapper::_stopProcess();
  return 0;
}

//------------------------------------------------------------------------
void Vst2Wrapper::setSampleRate(float newSamplerate) {
  BaseWrapper::_setSampleRate(newSamplerate);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setBlockSize(VstInt32 newBlockSize) {
  BaseWrapper::_setBlockSize(newBlockSize);
}

//------------------------------------------------------------------------
float Vst2Wrapper::getParameter(VstInt32 index) {
  return BaseWrapper::_getParameter(index);
}

//------------------------------------------------------------------------
void Vst2Wrapper::setParameter(VstInt32 index, float value) {
  if (!mController)
    return;

  if (index < (int32)mParameterMap.size()) {
    ParamID id = mParameterMap.at(index).vst3ID;
    addParameterChange(id, (ParamValue)value, 0);
  }
}

//------------------------------------------------------------------------
void Vst2Wrapper::setProgram(VstInt32 program) {
  if (mProgramParameterID != kNoParamId && mController != nullptr &&
      mProgramParameterIdx != -1) {
    curProgram = program;

    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(mProgramParameterIdx, paramInfo) ==
        kResultTrue) {
      if (paramInfo.stepCount > 0 && program <= paramInfo.stepCount) {
        ParamValue normalized =
            (ParamValue)program / (ParamValue)paramInfo.stepCount;
        addParameterChange(mProgramParameterID, normalized, 0);
      }
    }
  }
}

//------------------------------------------------------------------------
void Vst2Wrapper::setProgramName(char *name) {
  // not supported in VST 3
}

//------------------------------------------------------------------------
void Vst2Wrapper::getProgramName(char *name) {
  // name of the current program. Limited to #kVstMaxProgNameLen.
  *name = 0;
  if (mUnitInfo) {
    ProgramListInfo listInfo = {0};
    if (mUnitInfo->getProgramListInfo(0, listInfo) == kResultTrue) {
      String128 tmp = {0};
      if (mUnitInfo->getProgramName(listInfo.id, curProgram, tmp) ==
          kResultTrue) {
        String str(tmp);
        str.copyTo8(name, 0, kVstMaxProgNameLen);
      }
    }
  }
}

int Vst2Wrapper::getProgram() { return curProgram; }

//------------------------------------------------------------------------
bool Vst2Wrapper::getProgramNameIndexed(VstInt32, VstInt32 index, char *name) {
  *name = 0;
  if (mUnitInfo) {
    ProgramListInfo listInfo = {0};
    if (mUnitInfo->getProgramListInfo(0, listInfo) == kResultTrue) {
      String128 tmp = {0};
      if (mUnitInfo->getProgramName(listInfo.id, index, tmp) == kResultTrue) {
        String str(tmp);
        str.copyTo8(name, 0, kVstMaxProgNameLen);
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterLabel(VstInt32 index, char *label) {
  // units in which parameter \e index is displayed (i.e. "sec", "dB", "type",
  // etc...). Limited to #kVstMaxParamStrLen.
  *label = 0;
  if (mController) {
    int32 vst3Index = mParameterMap.at(index).vst3Index;

    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(vst3Index, paramInfo) == kResultTrue) {
      String str(paramInfo.units);
      str.copyTo8(label, 0, kVstMaxParamStrLen);
    }
  }
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterDisplay(VstInt32 index, char *text) {
  // string representation ("0.5", "-3", "PLATE", etc...) of the value of
  // parameter \e index. Limited to #kVstMaxParamStrLen.
  *text = 0;
  if (mController) {
    int32 vst3Index = mParameterMap.at(index).vst3Index;

    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(vst3Index, paramInfo) == kResultTrue) {
      String128 tmp = {0};
      ParamValue value = 0;
      if (!getLastParamChange(paramInfo.id, value))
        value = mController->getParamNormalized(paramInfo.id);

      if (mController->getParamStringByValue(paramInfo.id, value, tmp) ==
          kResultTrue) {
        String str(tmp);
        str.copyTo8(text, 0, kVstMaxParamStrLen);
      }
    }
  }
}

//------------------------------------------------------------------------
void Vst2Wrapper::getParameterName(VstInt32 index, char *text) {
  // name ("Time", "Gain", "RoomType", etc...) of parameter \e index. Limited to
  // #kVstExtMaxParamStrLen.
  *text = 0;
  if (mController && index < (int32)mParameterMap.size()) {
    int32 vst3Index = mParameterMap.at(index).vst3Index;

    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(vst3Index, paramInfo) == kResultTrue) {
      String str;
      if (vst2WrapperFullParameterPath) {
        //! The parameter's name contains the unit path (e.g. "LFO 1.freq") as
        //! well
        if (mUnitInfo) {
          getUnitPath(paramInfo.unitId, str);
        }
      }
      str.append(paramInfo.title);

      if (str.length() > kVstExtMaxParamStrLen) {
        //! In case the string's length exceeds the limit, try parameter's title
        //! without
        // unit path.
        str = paramInfo.title;
      }
      if (str.length() > kVstExtMaxParamStrLen) {
        str = paramInfo.shortTitle;
      }
      str.copyTo8(text, 0, kVstExtMaxParamStrLen);
    }
  }
}

//------------------------------------------------------------------------
bool Vst2Wrapper::canParameterBeAutomated(VstInt32 index) {
  if (mController && index < (int32)mParameterMap.size()) {
    int32 vst3Index = mParameterMap.at(index).vst3Index;

    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(vst3Index, paramInfo) == kResultTrue)
      return (paramInfo.flags & ParameterInfo::kCanAutomate) != 0;
  }
  return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::string2parameter(VstInt32 index, char *text) {
  if (mController && index < (int32)mParameterMap.size()) {
    int32 vst3Index = mParameterMap.at(index).vst3Index;

    ParameterInfo paramInfo = {0};
    if (mController->getParameterInfo(vst3Index, paramInfo) == kResultTrue) {
      TChar tString[1024] = {0};
      String tmp(text);
      tmp.copyTo16(tString, 0, 1023);

      ParamValue valueNormalized = 0.0;

      if (mController->getParamValueByString(paramInfo.id, tString,
                                             valueNormalized)) {

        setParameter(index, (float)valueNormalized);
        if (audioMaster)
          audioMaster(0, audioMasterAutomate, index, 0, 0,
                      (float)valueNormalized);
        //	setParameterAutomated (index, (float)valueNormalized);
        // TODO: check if setParameterAutomated is correct
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getChunk(void **data, bool isPreset) {
  return BaseWrapper::_getChunk(data, isPreset);
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::setChunk(void *data, VstInt32 byteSize, bool isPreset) {
  return BaseWrapper::_setChunk(data, byteSize, isPreset);
}

//------------------------------------------------------------------------
bool Vst2Wrapper::setBypass(bool onOff) {
  return BaseWrapper::_setBypass(onOff);
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::setProcessPrecision(VstInt32 precision) {
  int32 newVst3SampleSize = -1;

  if (precision == kVstProcessPrecision32)
    newVst3SampleSize = kSample32;
  else if (precision == kVstProcessPrecision64)
    newVst3SampleSize = kSample64;

  if (newVst3SampleSize != mVst3SampleSize) {
    if (mProcessor &&
        mProcessor->canProcessSampleSize(newVst3SampleSize) == kResultTrue) {
      mVst3SampleSize = newVst3SampleSize;
      setupProcessing();

      setupBuses();

      return true;
    }
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getNumMidiInputChannels() {
  if (!mComponent)
    return 0;

  int32 busCount = mComponent->getBusCount(kEvent, kInput);
  if (busCount > 0) {
    BusInfo busInfo = {0};
    if (mComponent->getBusInfo(kEvent, kInput, 0, busInfo) == kResultTrue) {
      return busInfo.channelCount;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getNumMidiOutputChannels() {
  if (!mComponent)
    return 0;

  int32 busCount = mComponent->getBusCount(kEvent, kOutput);
  if (busCount > 0) {
    BusInfo busInfo = {0};
    if (mComponent->getBusInfo(kEvent, kOutput, 0, busInfo) == kResultTrue) {
      return busInfo.channelCount;
    }
  }
  return 0;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getGetTailSize() {
  if (mProcessor)
    return mProcessor->getTailSamples();

  return 0;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getEffectName(char *effectName) {
  if (mName[0]) {
    strncpy(effectName, mName, kVstMaxEffectNameLen);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::getVendorString(char *text) {
  if (mVendor[0]) {
    strncpy(text, mVendor, kVstMaxVendorStrLen);
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::getVendorVersion() { return mVersion; }

//-----------------------------------------------------------------------------
VstIntPtr Vst2Wrapper::vendorSpecific(VstInt32 lArg, VstIntPtr lArg2,
                                      void *ptrArg, float floatArg) {
  switch (lArg) {
  case 'stCA':
  case 'stCa':
    switch (lArg2) {
    //--- -------
    case 'FUID':
      if (ptrArg && mVst3EffectClassID.isValid()) {
        memcpy((char *)ptrArg, mVst3EffectClassID, 16);
        return 1;
      }
      break;
    }
  }
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::canDo(char *text) {
  if (stricmp(text, "sendVstEvents") == 0) {
    return -1;
  } else if (stricmp(text, "sendVstMidiEvent") == 0) {
    return mHasEventOutputBuses ? 1 : -1;
  } else if (stricmp(text, "receiveVstEvents") == 0) {
    return -1;
  } else if (stricmp(text, "receiveVstMidiEvent") == 0) {
    return mHasEventInputBuses ? 1 : -1;
  } else if (stricmp(text, "receiveVstTimeInfo") == 0) {
    return 1;
  } else if (stricmp(text, "offline") == 0) {
    if (mProcessing)
      return 0;
    if (mVst3processMode == kOffline)
      return 1;

    bool canOffline = setupProcessing(kOffline);
    setupProcessing();
    return canOffline ? 1 : -1;
  } else if (stricmp(text, "midiProgramNames") == 0) {
    if (mUnitInfo) {
      UnitID unitId = -1;
      if (mUnitInfo->getUnitByBus(kEvent, kInput, 0, 0, unitId) ==
              kResultTrue &&
          unitId >= 0)
        return 1;
    }
    return -1;
  } else if (stricmp(text, "bypass") == 0) {
    return mBypassParameterID != kNoParamId ? 1 : -1;
  }
  return 0; // do not know
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::setupParameters() {
  BaseWrapper::setupParameters();
  //	cEffect.numParams = mNumParams;
  numparams = mNumParams;
}

//-----------------------------------------------------------------------------
VstInt32 Vst2Wrapper::processEvents(VstEvents *events) {
  if (mInputEvents == nullptr)
    return 0;
  mInputEvents->clear();

  for (int32 i = 0; i < events->numEvents; i++) {
    VstEvent *e = events->events[i];
    if (e->type == kVstMidiType) {
      auto *midiEvent = (VstMidiEvent *)e;
      Event toAdd = {0, midiEvent->deltaFrames, 0};
      processMidiEvent(
          toAdd, &midiEvent->midiData[0],
          midiEvent->flags & kVstMidiEventIsRealtime, midiEvent->noteLength,
          midiEvent->noteOffVelocity * kMidiScaler, midiEvent->detune);
    }
    //--- -----------------------------
  }

  return 0;
}

//-----------------------------------------------------------------------------
inline void Vst2Wrapper::processOutputEvents() {
  if (mVst2OutputEvents && mOutputEvents &&
      mOutputEvents->getEventCount() > 0) {
    mVst2OutputEvents->flush();

    Event e = {0};
    for (int32 i = 0, total = mOutputEvents->getEventCount(); i < total; i++) {
      if (mOutputEvents->getEvent(i, e) != kResultOk)
        break;

      VstMidiEvent midiEvent = {0};
      midiEvent.deltaFrames = e.sampleOffset;
      if (e.flags & Event::kIsLive)
        midiEvent.flags = kVstMidiEventIsRealtime;
      char *midiData = midiEvent.midiData;

      switch (e.type) {
      //--- ---------------------
      case Event::kNoteOnEvent:
        midiData[0] = (char)(kNoteOn | (e.noteOn.channel & kChannelMask));
        midiData[1] = (char)(e.noteOn.pitch & kDataMask);
        midiData[2] =
            (char)((int32)(e.noteOn.velocity * 127.f + 0.4999999f) & kDataMask);
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
            (char)((int32)(e.noteOff.velocity * 127.f + 0.4999999f) &
                   kDataMask);
        break;

        break;
      }

      if (!mVst2OutputEvents->add(midiEvent))
        break;
    }

    mOutputEvents->clear();

    //	sendVstEventsToHost (*mVst2OutputEvents);

    if (audioMaster)
      audioMaster(0, audioMasterProcessEvents, 0, 0, *mVst2OutputEvents, 0);
  }
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::updateProcessLevel() {
  int32 currentLevel;

  currentLevel = 0;

  if (audioMaster)
    currentLevel =
        audioMaster(0, audioMasterGetCurrentProcessLevel, 0, 0, 0, 0);

  // getCurrentProcessLevel ();
  if (mCurrentProcessLevel != currentLevel) {
    mCurrentProcessLevel = currentLevel;
    //	if (mCurrentProcessLevel == kVstProcessLevelOffline)
    //		mVst3processMode = kOffline;
    //	else
    mVst3processMode = kRealtime;

    bool callStartStop = mProcessing;

    if (callStartStop)
      stopProcess();

    setupProcessing();

    if (callStartStop)
      startProcess();
  }
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::processReplacing(float **inputs, float **outputs,
                                   VstInt32 sampleFrames) {
  updateProcessLevel();

  _processReplacing(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::processDoubleReplacing(double **inputs, double **outputs,
                                         VstInt32 sampleFrames) {
  updateProcessLevel();

  _processDoubleReplacing(inputs, outputs, sampleFrames);
}

//-----------------------------------------------------------------------------
// static
//-----------------------------------------------------------------------------
Vst2Wrapper *Vst2Wrapper::create(IPluginFactory *factory,
                                 const TUID vst3ComponentID, VstInt32 vst2ID,
                                 audioMasterCallback audioMaster) {
  if (!factory)
    return nullptr;

  BaseWrapper::SVST3Config config;
  config.factory = factory;
  config.processor = nullptr;
  config.component = nullptr;

  FReleaser factoryReleaser(factory);

  config.factory = factory;

  config.vst3ComponentID = FUID::fromTUID(vst3ComponentID);

  auto *wrapper = new Vst2Wrapper(config, audioMaster, vst2ID);

  if (!wrapper)
    return nullptr;

  if (wrapper->init(audioMaster) == false) {
    wrapper->release();
    return nullptr;
  }

  FUnknownPtr<IPluginFactory2> factory2(factory);
  if (factory2) {
    PFactoryInfo factoryInfo;
    if (factory2->getFactoryInfo(&factoryInfo) == kResultTrue)
      wrapper->setVendorName(factoryInfo.vendor);

    for (int32 i = 0; i < factory2->countClasses(); i++) {
      Steinberg::PClassInfo2 classInfo2;
      if (factory2->getClassInfo2(i, &classInfo2) == Steinberg::kResultTrue) {
        if (memcmp(classInfo2.cid, vst3ComponentID, sizeof(TUID)) == 0) {
          wrapper->setSubCategories(classInfo2.subCategories);
          wrapper->setEffectName(classInfo2.name);
          wrapper->setEffectVersion(classInfo2.version);

          if (classInfo2.vendor[0] != 0)
            wrapper->setVendorName(classInfo2.vendor);

          break;
        }
      }
    }
  }

  if (strstr(wrapper->mSubCategories, "Instrument"))
    wrapper->synth = true;

  return wrapper;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::getName(String128 name) {
  char8 productString[128];
  int retval;

  retval = 0;

  if (audioMaster)
    retval =
        audioMaster(0, audioMasterGetProductString, 0, 0, productString, 0);

  // if (getHostProductString (productString))
  if (retval) {
    String str(productString);
    str.copyTo16(name, 0, 127);

    return kResultTrue;
  }
  return kResultFalse;
}

//-----------------------------------------------------------------------------
// IComponentHandler
//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::beginEdit(ParamID tag) {
  std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find(tag);
  if (iter != mParamIndexMap.end()) {
    if (audioMaster)
      audioMaster(0, audioMasterBeginEdit, (*iter).second, 0, 0, 0);
  }
  return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::performEdit(ParamID tag,
                                            ParamValue valueNormalized) {
  std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find(tag);
  if (iter != mParamIndexMap.end() && audioMaster) {
    if (audioMaster)
      audioMaster(0, audioMasterAutomate, (*iter).second, 0, nullptr,
                  (float)valueNormalized); // value is in opt
  }

  mInputTransfer.addChange(tag, valueNormalized, 0);

  return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API Vst2Wrapper::endEdit(ParamID tag) {
  std::map<ParamID, int32>::const_iterator iter = mParamIndexMap.find(tag);
  if (iter != mParamIndexMap.end()) {
    if (audioMaster)
      audioMaster(0, audioMasterEndEdit, (*iter).second, 0, 0, 0);
  }
  return kResultTrue;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_ioChanged() {
  AEffect plugin;

  BaseWrapper::_ioChanged();

  plugin.numPrograms = numprograms;
  plugin.numParams = numparams;
  plugin.numInputs = numinputs;
  plugin.numOutputs = numoutputs;
  plugin.initialDelay = initialdelay;

  if (audioMaster)
    audioMaster(&plugin, audioMasterIOChanged, 0, 0, 0, 0);

  //	ioChanged ();
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_updateDisplay() {
  BaseWrapper::_updateDisplay();
  //	updateDisplay ();
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_setNumInputs(int32 inputs) {
  BaseWrapper::_setNumInputs(inputs);

  //	setNumInputs (inputs);
  numinputs = inputs;
}

//-----------------------------------------------------------------------------
void Vst2Wrapper::_setNumOutputs(int32 outputs) {
  BaseWrapper::_setNumOutputs(outputs);
  //	setNumOutputs (outputs);
  numoutputs = outputs;
}

//-----------------------------------------------------------------------------
bool Vst2Wrapper::_sizeWindow(int32 width, int32 height) {
  //	return sizeWindow (width, height);
}

#ifndef VESTIGE
//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::vst3ToVst2SpeakerArr(SpeakerArrangement vst3Arr) {
  switch (vst3Arr) {
  case SpeakerArr::kMono:
    return kSpeakerArrMono;
  case SpeakerArr::kStereo:
    return kSpeakerArrStereo;
  case SpeakerArr::kStereoSurround:
    return kSpeakerArrStereoSurround;
  case SpeakerArr::kStereoCenter:
    return kSpeakerArrStereoCenter;
  case SpeakerArr::kStereoSide:
    return kSpeakerArrStereoSide;
  case SpeakerArr::kStereoCLfe:
    return kSpeakerArrStereoCLfe;
  case SpeakerArr::k30Cine:
    return kSpeakerArr30Cine;
  case SpeakerArr::k30Music:
    return kSpeakerArr30Music;
  case SpeakerArr::k31Cine:
    return kSpeakerArr31Cine;
  case SpeakerArr::k31Music:
    return kSpeakerArr31Music;
  case SpeakerArr::k40Cine:
    return kSpeakerArr40Cine;
  case SpeakerArr::k40Music:
    return kSpeakerArr40Music;
  case SpeakerArr::k41Cine:
    return kSpeakerArr41Cine;
  case SpeakerArr::k41Music:
    return kSpeakerArr41Music;
  case SpeakerArr::k50:
    return kSpeakerArr50;
  case SpeakerArr::k51:
    return kSpeakerArr51;
  case SpeakerArr::k60Cine:
    return kSpeakerArr60Cine;
  case SpeakerArr::k60Music:
    return kSpeakerArr60Music;
  case SpeakerArr::k61Cine:
    return kSpeakerArr61Cine;
  case SpeakerArr::k61Music:
    return kSpeakerArr61Music;
  case SpeakerArr::k70Cine:
    return kSpeakerArr70Cine;
  case SpeakerArr::k70Music:
    return kSpeakerArr70Music;
  case SpeakerArr::k71Cine:
    return kSpeakerArr71Cine;
  case SpeakerArr::k71Music:
    return kSpeakerArr71Music;
  case SpeakerArr::k80Cine:
    return kSpeakerArr80Cine;
  case SpeakerArr::k80Music:
    return kSpeakerArr80Music;
  case SpeakerArr::k81Cine:
    return kSpeakerArr81Cine;
  case SpeakerArr::k81Music:
    return kSpeakerArr81Music;
  case SpeakerArr::k102:
    return kSpeakerArr102;
  }

  return kSpeakerArrUserDefined;
}

//------------------------------------------------------------------------
SpeakerArrangement Vst2Wrapper::vst2ToVst3SpeakerArr(VstInt32 vst2Arr) {
  switch (vst2Arr) {
  case kSpeakerArrMono:
    return SpeakerArr::kMono;
  case kSpeakerArrStereo:
    return SpeakerArr::kStereo;
  case kSpeakerArrStereoSurround:
    return SpeakerArr::kStereoSurround;
  case kSpeakerArrStereoCenter:
    return SpeakerArr::kStereoCenter;
  case kSpeakerArrStereoSide:
    return SpeakerArr::kStereoSide;
  case kSpeakerArrStereoCLfe:
    return SpeakerArr::kStereoCLfe;
  case kSpeakerArr30Cine:
    return SpeakerArr::k30Cine;
  case kSpeakerArr30Music:
    return SpeakerArr::k30Music;
  case kSpeakerArr31Cine:
    return SpeakerArr::k31Cine;
  case kSpeakerArr31Music:
    return SpeakerArr::k31Music;
  case kSpeakerArr40Cine:
    return SpeakerArr::k40Cine;
  case kSpeakerArr40Music:
    return SpeakerArr::k40Music;
  case kSpeakerArr41Cine:
    return SpeakerArr::k41Cine;
  case kSpeakerArr41Music:
    return SpeakerArr::k41Music;
  case kSpeakerArr50:
    return SpeakerArr::k50;
  case kSpeakerArr51:
    return SpeakerArr::k51;
  case kSpeakerArr60Cine:
    return SpeakerArr::k60Cine;
  case kSpeakerArr60Music:
    return SpeakerArr::k60Music;
  case kSpeakerArr61Cine:
    return SpeakerArr::k61Cine;
  case kSpeakerArr61Music:
    return SpeakerArr::k61Music;
  case kSpeakerArr70Cine:
    return SpeakerArr::k70Cine;
  case kSpeakerArr70Music:
    return SpeakerArr::k70Music;
  case kSpeakerArr71Cine:
    return SpeakerArr::k71Cine;
  case kSpeakerArr71Music:
    return SpeakerArr::k71Music;
  case kSpeakerArr80Cine:
    return SpeakerArr::k80Cine;
  case kSpeakerArr80Music:
    return SpeakerArr::k80Music;
  case kSpeakerArr81Cine:
    return SpeakerArr::k81Cine;
  case kSpeakerArr81Music:
    return SpeakerArr::k81Music;
  case kSpeakerArr102:
    return SpeakerArr::k102;
  }

  return 0;
}

//------------------------------------------------------------------------
VstInt32 Vst2Wrapper::vst3ToVst2Speaker(Vst::Speaker vst3Speaker) {
  switch (vst3Speaker) {
  case Vst::kSpeakerM:
    return ::kSpeakerM;
  case Vst::kSpeakerL:
    return ::kSpeakerL;
  case Vst::kSpeakerR:
    return ::kSpeakerR;
  case Vst::kSpeakerC:
    return ::kSpeakerC;
  case Vst::kSpeakerLfe:
    return ::kSpeakerLfe;
  case Vst::kSpeakerLs:
    return ::kSpeakerLs;
  case Vst::kSpeakerRs:
    return ::kSpeakerRs;
  case Vst::kSpeakerLc:
    return ::kSpeakerLc;
  case Vst::kSpeakerRc:
    return ::kSpeakerRc;
  case Vst::kSpeakerS:
    return ::kSpeakerS;
  case Vst::kSpeakerSl:
    return ::kSpeakerSl;
  case Vst::kSpeakerSr:
    return ::kSpeakerSr;
  case Vst::kSpeakerTc:
    return ::kSpeakerTm;
  case Vst::kSpeakerTfl:
    return ::kSpeakerTfl;
  case Vst::kSpeakerTfc:
    return ::kSpeakerTfc;
  case Vst::kSpeakerTfr:
    return ::kSpeakerTfr;
  case Vst::kSpeakerTrl:
    return ::kSpeakerTrl;
  case Vst::kSpeakerTrc:
    return ::kSpeakerTrc;
  case Vst::kSpeakerTrr:
    return ::kSpeakerTrr;
  case Vst::kSpeakerLfe2:
    return ::kSpeakerLfe2;
  }
  return ::kSpeakerUndefined;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::pinIndexToBusChannel(BusDirection dir, VstInt32 pinIndex,
                                       int32 &busIndex, int32 &busChannel) {
  AudioBusBuffers *busBuffers =
      dir == kInput ? mProcessData.inputs : mProcessData.outputs;
  int32 busCount =
      dir == kInput ? mProcessData.numInputs : mProcessData.numOutputs;
  uint64 mainBusFlags =
      dir == kInput ? mMainAudioInputBuses : mMainAudioOutputBuses;

  int32 sourceIndex = 0;
  for (busIndex = 0; busIndex < busCount; busIndex++) {
    AudioBusBuffers &buffers = busBuffers[busIndex];
    if (mainBusFlags & (uint64(1) << busIndex)) {
      for (busChannel = 0; busChannel < buffers.numChannels; busChannel++) {
        if (pinIndex == sourceIndex) {
          return true;
        }
        sourceIndex++;
      }
    }
  }
  return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getPinProperties(BusDirection dir, VstInt32 pinIndex,
                                   VstPinProperties *properties) {
  int32 busIndex = -1;
  int32 busChannelIndex = -1;

  if (pinIndexToBusChannel(dir, pinIndex, busIndex, busChannelIndex)) {
    BusInfo busInfo = {0};
    if (mComponent &&
        mComponent->getBusInfo(kAudio, dir, busIndex, busInfo) == kResultTrue) {
      properties->flags = kVstPinIsActive; // ????

      String name(busInfo.name);
      name.copyTo8(properties->label, 0, kVstMaxLabelLen);

      if (busInfo.channelCount == 1) {
        properties->flags |= kVstPinUseSpeaker;
        properties->arrangementType = kSpeakerArrMono;
      }
      if (busInfo.channelCount == 2) {
        properties->flags |= kVstPinUseSpeaker;
        properties->flags |= kVstPinIsStereo;
        properties->arrangementType = kSpeakerArrStereo;
      } else if (busInfo.channelCount > 2) {
        Vst::SpeakerArrangement arr = 0;
        if (mProcessor &&
            mProcessor->getBusArrangement(dir, busIndex, arr) == kResultTrue) {
          properties->flags |= kVstPinUseSpeaker;
          properties->arrangementType = vst3ToVst2SpeakerArr(arr);
        } else {
          return false;
        }
      }
      return true;
    }
  }

  return false;
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getInputProperties(VstInt32 index,
                                     VstPinProperties *properties) {
  return getPinProperties(kInput, index, properties);
}

//------------------------------------------------------------------------
bool Vst2Wrapper::getOutputProperties(VstInt32 index,
                                      VstPinProperties *properties) {
  return getPinProperties(kOutput, index, properties);
}
#endif

//-----------------------------------------------------------------------------
} // namespace Vst
} // namespace Steinberg

// extern bool InitModule();

