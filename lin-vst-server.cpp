/*  dssi-vst: a DSSI plugin wrapper for VST effects and instruments
    Copyright 2004-2007 Chris Cannam

    This file is part of linvst.

    linvst is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <time.h>

#include <sched.h>

#define stricmp strcasecmp
#define strnicmp strncasecmp

/*
#ifdef __WINE__
//#else
#define __cdecl
#endif
*/

/*

#if VST_FORCE_DEPRECATED
#define DEPRECATED_VST_SYMBOL(x) __##x##Deprecated
#else
#define DEPRECATED_VST_SYMBOL(x) x
#endif

*/

#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include "remotepluginserver.h"

#include "paths.h"

#include "vst2wrapper.sdk.cpp"
#include "vst2wrapper.h"
#include "public.sdk/source/vst/hosting/module.h"

extern "C"
{
	typedef bool (PLUGIN_API *InitModuleProc) ();
	typedef bool (PLUGIN_API *ExitModuleProc) ();
}
static const Steinberg::FIDString kInitModuleProcName = "InitDll";
static const Steinberg::FIDString kExitModuleProcName = "ExitDll";

#define APPLICATION_CLASS_NAME "dssi_vst"
#ifdef TRACKTIONWM 
#define APPLICATION_CLASS_NAME2 "dssi_vst2"
#endif

RemotePluginDebugLevel  debugLevel = RemotePluginDebugNone;

#define disconnectserver 32143215

#define hidegui2 77775634

using namespace std;

class RemoteVSTServer : public RemotePluginServer
{
public:
                        RemoteVSTServer(std::string fileIdentifiers, std::string fallbackName);
    virtual             ~RemoteVSTServer();

     // virtual std::string getName() { return m_name; }
     // virtual std::string getMaker() { return m_maker; }
    virtual std::string getName();
    virtual std::string getMaker();
    
    virtual void        setBufferSize(int);
    virtual void        setSampleRate(int);
    virtual void        reset();
    virtual void        terminate();

    virtual int         getInputCount() { return vst2wrap->numinputs; }
    virtual int         getOutputCount() { return vst2wrap->numoutputs; }
    virtual int         getFlags() { if(vst2wrap->synth == true)
    mpluginptr->flags |= effFlagsIsSynth; 
    #ifdef DOUBLEP
    if(vst2wrap->doublereplacing == true)
    mpluginptr->flags |= effFlagsCanDoubleReplacing; 
    #endif
    mpluginptr->flags |= effFlagsCanReplacing; mpluginptr->flags |= effFlagsProgramChunks; return mpluginptr->flags; }
    virtual int         getinitialDelay() { return vst2wrap->initialdelay; }
    virtual int         getUID() { return vst2uid; }
    virtual int         getParameterCount() { return vst2wrap->numparams; }
    virtual std::string getParameterName(int);
    virtual void        setParameter(int, float);
    virtual float       getParameter(int);
    virtual void        getParameters(int, int, float *);

    virtual int         getProgramCount() { return vst2wrap->numprograms; }
    virtual int         getProgramNameIndexed(int, char *name);
    virtual std::string getProgramName();

    virtual void        setCurrentProgram(int);

    virtual void        showGUI();
    virtual void        hideGUI();
#ifdef EMBED
    virtual void        openGUI();
#endif
    virtual void        guiUpdate();
    
    virtual int         getEffInt(int opcode);
    virtual std::string getEffString(int opcode, int index);
    virtual void        effDoVoid(int opcode);
    virtual int         effDoVoid2(int opcode, int index, int value, float opt);
  
//    virtual int         getInitialDelay() {return m_plugin->initialDelay;}
//    virtual int         getUniqueID() { return m_plugin->uniqueID;}
//    virtual int         getVersion() { return m_plugin->version;}

    virtual int         processVstEvents();
    virtual void        getChunk();
    virtual void        setChunk();
    virtual void        canBeAutomated();
    virtual void        getProgram();
    virtual void        EffectOpen();
//    virtual void        eff_mainsChanged(int v);

    virtual void        process(float **inputs, float **outputs, int sampleFrames);
#ifdef DOUBLEP
    virtual void        processdouble(double **inputs, double **outputs, int sampleFrames);
    virtual bool        setPrecision(int);  
#endif
#ifdef MIDIEFF
    virtual bool        getOutProp(int);
    virtual bool        getInProp(int);
    virtual bool        getMidiKey(int);
    virtual bool        getMidiProgName(int);
    virtual bool        getMidiCurProg(int);
    virtual bool        getMidiProgCat(int);
    virtual bool        getMidiProgCh(int);
    virtual bool        setSpeaker();
    virtual bool        getSpeaker();
#endif
#ifdef CANDOEFF 
    virtual bool        getEffCanDo(std::string);
#endif 

    virtual void        setDebugLevel(RemotePluginDebugLevel level) { debugLevel = level; }

    virtual bool        warn(std::string);

    virtual void        waitForServer();
    virtual void        waitForServerexit();

    HWND                hWnd;
    WNDCLASSEX          wclass;
#ifdef TRACKTIONWM  
    WNDCLASSEX          wclass2;
#endif    
    UINT_PTR            timerval;
    bool                haveGui;
#ifdef EMBED
    HANDLE handlewin;
    winmessage *winm;
#endif
    int guiupdate;
    int guiupdatecount;
    int guiresizewidth;
    int guiresizeheight;
    ERect               *rect;
    int                 setprogrammiss;
    int                 hostreaper;
    int                 melda;      
    int                 wavesthread;
#ifdef EMBED
#ifdef TRACKTIONWM  
    int                 hosttracktion;
#endif
#endif
    AEffect             mplugin;
    AEffect*            mpluginptr;
    VstEvents           vstev[VSTSIZE];
    int                 bufferSize;
    int                 sampleRate;
    bool                exiting;
    bool                effectrun;
    bool                inProcessThread;
    bool                guiVisible;
    int                 parfin;
    int                 audfin;
    int                 getfin;
    int                 hidegui;    
    
    Steinberg::Vst::Vst2Wrapper *vst2wrap;
    int vst2uid;
    Steinberg::IPluginFactory* factory;

private:
    std::string         m_name;
    std::string         m_maker;
};

RemoteVSTServer         *remoteVSTServerInstance = 0;


LRESULT WINAPI MainProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
#ifndef EMBED
         if(remoteVSTServerInstance)
	 {
         if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->guiVisible)
	 {
         remoteVSTServerInstance->hideGUI();	 
         return 0;
	 }
         }
#endif
    break;
		    
     case WM_TIMER:
     break;
       	
    default:
    return DefWindowProc(hWnd, msg, wParam, lParam);		   
    }
 return 0;
}

#ifdef TRACKTIONWM 
LRESULT WINAPI MainProc2(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
    break;
		    
    case WM_TIMER:
    break;
       	
    default:
    return DefWindowProc(hWnd, msg, wParam, lParam);		   
    }
 return 0;
}
#endif

DWORD WINAPI AudioThreadMain(LPVOID parameter)
{
/*
    struct sched_param param;
    param.sched_priority = 1;

    int result = sched_setscheduler(0, SCHED_FIFO, &param);

    if (result < 0)
    {
        perror("Failed to set realtime priority for audio thread");
    }
*/
    while (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->dispatchProcess(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
    remoteVSTServerInstance->audfin = 1;
    ExitThread(0);
    return 0;
}

DWORD WINAPI GetSetThreadMain(LPVOID parameter)
{
/*
    struct sched_param param;
    param.sched_priority = 1;

    int result = sched_setscheduler(0, SCHED_FIFO, &param);

    if (result < 0)
    {
        perror("Failed to set realtime priority for audio thread");
    }
*/
    while (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->dispatchGetSet(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
    remoteVSTServerInstance->getfin = 1;
    ExitThread(0);
    return 0;
}

DWORD WINAPI ParThreadMain(LPVOID parameter)
{
/*
    struct sched_param param;
    param.sched_priority = 1;

    int result = sched_setscheduler(0, SCHED_FIFO, &param);

    if (result < 0)
    {
        perror("Failed to set realtime priority for audio thread");
    }
*/
    while (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->dispatchPar(50);
    }
    // param.sched_priority = 0;
    // (void)sched_setscheduler(0, SCHED_OTHER, &param);
     remoteVSTServerInstance->parfin = 1;
     ExitThread(0);
    return 0;
}

RemoteVSTServer::RemoteVSTServer(std::string fileIdentifiers, std::string fallbackName) :
    RemotePluginServer(fileIdentifiers),
    m_name(fallbackName),
    m_maker(""),
    bufferSize(0),
    sampleRate(0),
    setprogrammiss(0),
    hostreaper(0),
    wavesthread(1),
#ifdef EMBED
    winm(0),
#ifdef TRACKTIONWM  
    hosttracktion(0),
#endif
#endif
    haveGui(true),
    timerval(0),
    exiting(false),
    effectrun(false),
    inProcessThread(false),
    guiVisible(false),
    parfin(0),
    audfin(0),
    getfin(0),
    guiupdate(0),
    guiupdatecount(0),
    guiresizewidth(500),
    guiresizeheight(200),
    melda(0),
    hWnd(0),
    vst2wrap(0),
    factory(0),
    mpluginptr(&mplugin),
    vst2uid(0),
    hidegui(0)
{
#ifdef EMBED	    
     winm = new winmessage;  
     if(!winm)
     starterror = 1;
#endif	    
}

std::string RemoteVSTServer::getName()
{
      char buffer[512];
      memset(buffer, 0, sizeof(buffer));
      vst2wrap->getEffectName (buffer);  	
      if (buffer[0])
      {
      m_name = buffer; 	      
      if(strlen(buffer) < (kVstMaxEffectNameLen - 7))
      m_name = m_name + " [vst3]"; 
      }         	
      return m_name;
}
  
std::string RemoteVSTServer::getMaker()
{
      char buffer[512];
      memset(buffer, 0, sizeof(buffer));
      vst2wrap->getVendorString (buffer);
      if (buffer[0])
      m_maker = buffer;
      return m_maker;
}

void RemoteVSTServer::EffectOpen()
{
    if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: opening plugin" << endl;	
	
    vst2wrap->suspend ();

    char buffer[512];
    memset(buffer, 0, sizeof(buffer));
	
    string buffer2 = getMaker();
    strcpy(buffer, buffer2.c_str());

/*
    if (strncmp(buffer, "Guitar Rig 5", 12) == 0)
        setprogrammiss = 1;
    if (strncmp(buffer, "T-Rack", 6) == 0)
        setprogrammiss = 1;
*/
		
    if(strcmp("MeldaProduction", buffer) == 0)
    {
    melda = 1;	
    wavesthread = 1;
    }
	
#ifdef WAVES2
    if(strcmp("Waves", buffer) == 0)
    {
    mpluginptr->flags |= effFlagsHasEditor;
    haveGui = true;
    wavesthread = 1;
    }
#endif
/*
    if (strncmp(buffer, "IK", 2) == 0)
        setprogrammiss = 1;
*/	
#ifndef WCLASS
	    if (haveGui == true)
    {
	memset(&wclass, 0, sizeof(WNDCLASSEX));
        wclass.cbSize = sizeof(WNDCLASSEX);
        wclass.style = 0;
	    // CS_HREDRAW | CS_VREDRAW;
        wclass.lpfnWndProc = MainProc;
        wclass.cbClsExtra = 0;
        wclass.cbWndExtra = 0;
        wclass.hInstance = GetModuleHandle(0);
        wclass.hIcon = LoadIcon(GetModuleHandle(0), APPLICATION_CLASS_NAME);
        wclass.hCursor = LoadCursor(0, IDI_APPLICATION);
        // wclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wclass.lpszMenuName = "MENU_DSSI_VST";
        wclass.lpszClassName = APPLICATION_CLASS_NAME;
        wclass.hIconSm = 0;

        if (!RegisterClassEx(&wclass))
        {
            cerr << "dssi-vst-server: ERROR: Failed to register Windows application class!\n" << endl;
            haveGui = false;
        }
#ifdef TRACKTIONWM       
    	memset(&wclass2, 0, sizeof(WNDCLASSEX));
        wclass2.cbSize = sizeof(WNDCLASSEX);
        wclass2.style = 0;
	    // CS_HREDRAW | CS_VREDRAW;
        wclass2.lpfnWndProc = MainProc2;
        wclass2.cbClsExtra = 0;
        wclass2.cbWndExtra = 0;
        wclass2.hInstance = GetModuleHandle(0);
        wclass2.hIcon = LoadIcon(GetModuleHandle(0), APPLICATION_CLASS_NAME2);
        wclass2.hCursor = LoadCursor(0, IDI_APPLICATION);
        // wclass2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wclass2.lpszMenuName = "MENU_DSSI_VST2";
        wclass2.lpszClassName = APPLICATION_CLASS_NAME2;
        wclass2.hIconSm = 0;

        if (!RegisterClassEx(&wclass2))
        {
            cerr << "dssi-vst-server: ERROR: Failed to register Windows application class!\n" << endl;
            haveGui = false;
        }
#endif        
    }
#endif

    struct amessage
    {
        int flags;
        int pcount;
        int parcount;
        int incount;
        int outcount;
        int delay;
    } am;

     //   am.flags = plugin->flags;
        am.flags = getFlags();
        // remoteVSTServerInstance->mpluginptr->flags;
        am.pcount = getProgramCount();
        am.parcount = getParameterCount();
        am.incount = getInputCount();
        am.outcount = getOutputCount();
        am.delay = getinitialDelay();
#ifdef DOUBLEP
        if(remoteVSTServerInstance->vst2wrap->doublereplacing == true)
        am.flags |= effFlagsCanDoubleReplacing; 
        am.flags |= effFlagsCanReplacing; 
#else
        am.flags &= ~effFlagsCanDoubleReplacing;
        am.flags |= effFlagsCanReplacing; 
#endif        

        memcpy(&remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], &am, sizeof(am));

    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)audioMasterIOChanged);
   
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();	
	
    vst2wrap->resume ();	
	
    effectrun = true;	
}

RemoteVSTServer::~RemoteVSTServer()
{
    if(effectrun == true)
    {	 
    vst2wrap->suspend ();
    } 
    
    if(vst2wrap)
    delete vst2wrap;  
	
    if (factory)
    factory->release ();      
	
#ifdef EMBED	
    if (winm)
    delete winm; 
#endif	
}

void RemoteVSTServer::process(float **inputs, float **outputs, int sampleFrames)
{
    inProcessThread = true;
 
    vst2wrap->processReplacing (inputs, outputs, sampleFrames);
 
    inProcessThread = false;
}

#ifdef DOUBLEP
void RemoteVSTServer::processdouble(double **inputs, double **outputs, int sampleFrames)
{
    inProcessThread = true;

    vst2wrap->processDoubleReplacing(inputs, outputs, sampleFrames);

    inProcessThread = false;
}

bool RemoteVSTServer::setPrecision(int value)
{
bool retval;

     retval = vst2wrap->setProcessPrecision(value);

     return retval;       
}    
#endif

#ifdef MIDIEFF
bool RemoteVSTServer::getInProp(int index)
{
VstPinProperties ptr;
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effGetInputProperties, index, 0, &ptr, 0);

        tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstPinProperties)], &ptr, sizeof(VstPinProperties));

        return retval;       
}

bool RemoteVSTServer::getOutProp(int index)
{
VstPinProperties ptr;
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effGetOutputProperties, index, 0, &ptr, 0);

        tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstPinProperties)], &ptr, sizeof(VstPinProperties));

        return retval;         
}

bool RemoteVSTServer::getMidiKey(int index)
{
MidiKeyName ptr;
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effGetMidiKeyName, index, 0, &ptr, 0);
        tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiKeyName)], &ptr, sizeof(MidiKeyName));

        return retval;       
}

bool RemoteVSTServer::getMidiProgName(int index)
{
MidiProgramName ptr;
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effGetMidiProgramName, index, 0, &ptr, 0);
        tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiProgramName)], &ptr, sizeof(MidiProgramName));

        return retval;       
}

bool RemoteVSTServer::getMidiCurProg(int index)
{
MidiProgramName ptr;
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effGetCurrentMidiProgram, index, 0, &ptr, 0);
        tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiProgramName)], &ptr, sizeof(MidiProgramName));

        return retval;       
}

bool RemoteVSTServer::getMidiProgCat(int index)
{
MidiProgramCategory ptr;
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effGetMidiProgramCategory, index, 0, &ptr, 0);
        tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(MidiProgramCategory)], &ptr, sizeof(MidiProgramCategory));

        return retval;       
}

bool RemoteVSTServer::getMidiProgCh(int index)
{
bool retval;

        retval = m_plugin->dispatcher(m_plugin, effHasMidiProgramsChanged, index, 0, 0, 0);

        return retval;       
}

bool RemoteVSTServer::setSpeaker() 
{
VstSpeakerArrangement ptr;
VstSpeakerArrangement value;
bool retval;

       tryRead(&m_shm2[FIXED_SHM_SIZE2 - (sizeof(VstSpeakerArrangement)*2)], &ptr, sizeof(VstSpeakerArrangement));

       tryRead(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstSpeakerArrangement)], &value, sizeof(VstSpeakerArrangement));

       retval = m_plugin->dispatcher(m_plugin, effSetSpeakerArrangement, 0, (VstIntPtr)&value, &ptr, 0);

       return retval;  
}       

bool RemoteVSTServer::getSpeaker() 
{
VstSpeakerArrangement ptr;
VstSpeakerArrangement value;
bool retval;

       retval = m_plugin->dispatcher(m_plugin, effSetSpeakerArrangement, 0, (VstIntPtr)&value, &ptr, 0);

       tryWrite(&m_shm2[FIXED_SHM_SIZE2 - (sizeof(VstSpeakerArrangement)*2)], &ptr, sizeof(VstSpeakerArrangement));

       tryWrite(&m_shm2[FIXED_SHM_SIZE2 - sizeof(VstSpeakerArrangement)], &value, sizeof(VstSpeakerArrangement));
 
       return retval;  
}     
#endif	

#ifdef CANDOEFF
bool RemoteVSTServer::getEffCanDo(std::string ptr)
{
        if(m_plugin->dispatcher(m_plugin, effCanDo, 0, 0, (char*)ptr.c_str(), 0))
        return true;
        else
        return false;
}
#endif

int RemoteVSTServer::getEffInt(int opcode)
{
    if(vst2wrap->synth == true)
    return 2;
    else 
    return 1;
}

void RemoteVSTServer::effDoVoid(int opcode)
{
    if (opcode == 78345432)
    {
//        hostreaper = 1;
         return;
    }
#ifdef EMBED
#ifdef TRACKTIONWM  
    if (opcode == 67584930)
    {
        hosttracktion = 1;
         return;
    }	
#endif	
#endif
	
    if (opcode == effClose)
    {
         // usleep(500000);
        waitForServerexit();
        terminate();
	return;    
    }
    
    if(opcode == effStartProcess)
    vst2wrap->startProcess ();
    
    if(opcode == effStopProcess)
    vst2wrap->stopProcess ();
}

int RemoteVSTServer::effDoVoid2(int opcode, int index, int value, float opt)
{	
    if(opcode == hidegui2)
    {
    hidegui = 1;  
#ifdef XECLOSE    
    while(hidegui == 1)
    {    
    sched_yield();
    } 
#endif       
    }  		
	
    if(opcode == effMainsChanged)
    {
    if(value == 0)
    vst2wrap->suspend ();
    if(value == 1)
    vst2wrap->resume ();
    }
}

std::string RemoteVSTServer::getEffString(int opcode, int index)
{
    char name[512];
    memset(name, 0, sizeof(name));

    if(opcode == effGetParamDisplay)
    vst2wrap->getParameterDisplay (index, name);
 
    if(opcode == effGetParamLabel)
    vst2wrap->getParameterLabel (index, name);
    
    return name;
}

void RemoteVSTServer::setBufferSize(int sz)
{
    if (bufferSize != sz)
    {
    vst2wrap->suspend ();    
    vst2wrap->setBlockSize (sz);
    vst2wrap->resume ();
    bufferSize = sz;
    }
   
    if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: set buffer size to " << sz << endl;
}

void RemoteVSTServer::setSampleRate(int sr)
{
    if (sampleRate != sr)
    {
    vst2wrap->suspend ();
    vst2wrap->setSampleRate (sr);
    vst2wrap->resume ();
    sampleRate = sr;
    }

    if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: set sample rate to " << sr << endl;
}

void RemoteVSTServer::reset()
{
    cerr << "dssi-vst-server[1]: reset" << endl;
}

void RemoteVSTServer::terminate()
{
    exiting = true;	
	
  //  cerr << "RemoteVSTServer::terminate: setting exiting flag" << endl;
}

std::string RemoteVSTServer::getParameterName(int p)
{
    char name[512];
    memset(name, 0, sizeof(name));       
    vst2wrap->getParameterName(p, name);
    return name;
}

void RemoteVSTServer::setParameter(int p, float v)
{ 
    vst2wrap->setParameter(p, v);
}

float RemoteVSTServer::getParameter(int p)
{ 
    return vst2wrap->getParameter(p);
}

void RemoteVSTServer::getParameters(int p0, int pn, float *v)
{
}

int RemoteVSTServer::getProgramNameIndexed(int p, char *name)
{
    if (debugLevel > 1)
        cerr << "dssi-vst-server[2]: getProgramName(" << p << ")" << endl;

    int retval = 0;
    char nameret[512];
    memset(nameret, 0, sizeof(nameret));
    retval = vst2wrap->getProgramNameIndexed (0, p, nameret);    
    strcpy(name, nameret); 
    return retval;
}

std::string
RemoteVSTServer::getProgramName()
{
    if (debugLevel > 1)
        cerr << "dssi-vst-server[2]: getProgramName()" << endl;

    char name[512];
    memset(name, 0, sizeof(name));  
    vst2wrap->getProgramName (name);  
    return name;
}

void
RemoteVSTServer::setCurrentProgram(int p)
{
    if (debugLevel > 1)
        cerr << "dssi-vst-server[2]: setCurrentProgram(" << p << ")" << endl;

/*
    if ((hostreaper == 1) && (setprogrammiss == 1))
    {
        writeIntring(&m_shmControl5->ringBuffer, 1);
        return;
    }
*/

     if (p < vst2wrap->numprograms)
     vst2wrap->setProgram(p);
}

bool RemoteVSTServer::warn(std::string warning)
{
    if (hWnd)
        MessageBox(hWnd, warning.c_str(), "Error", 0);
    return true;
}

void RemoteVSTServer::showGUI()
{	
#ifdef WCLASS
        memset(&wclass, 0, sizeof(WNDCLASSEX));
        wclass.cbSize = sizeof(WNDCLASSEX);
        wclass.style = 0;
	    // CS_HREDRAW | CS_VREDRAW;
        wclass.lpfnWndProc = MainProc;
        wclass.cbClsExtra = 0;
        wclass.cbWndExtra = 0;
        wclass.hInstance = GetModuleHandle(0);
        wclass.hIcon = LoadIcon(GetModuleHandle(0), APPLICATION_CLASS_NAME);
        wclass.hCursor = LoadCursor(0, IDI_APPLICATION);
        // wclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wclass.lpszMenuName = "MENU_DSSI_VST";
        wclass.lpszClassName = APPLICATION_CLASS_NAME;
        wclass.hIconSm = 0;

        if (!RegisterClassEx(&wclass))
        {
        guiVisible = false;
#ifdef EMBED        
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));
#endif        
        return;
        }        
#ifdef EMBED         
#ifdef TRACKTIONWM       
    	memset(&wclass2, 0, sizeof(WNDCLASSEX));
        wclass2.cbSize = sizeof(WNDCLASSEX);
        wclass2.style = 0;
	    // CS_HREDRAW | CS_VREDRAW;
        wclass2.lpfnWndProc = MainProc2;
        wclass2.cbClsExtra = 0;
        wclass2.cbWndExtra = 0;
        wclass2.hInstance = GetModuleHandle(0);
        wclass2.hIcon = LoadIcon(GetModuleHandle(0), APPLICATION_CLASS_NAME2);
        wclass2.hCursor = LoadCursor(0, IDI_APPLICATION);
        // wclass2.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wclass2.lpszMenuName = "MENU_DSSI_VST2";
        wclass2.lpszClassName = APPLICATION_CLASS_NAME2;
        wclass2.hIconSm = 0;

        if (!RegisterClassEx(&wclass2))
        {       
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0)); 
        guiVisible = false;        
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));       
        return;
        }
#endif 
#endif
#endif       
	
#ifdef EMBED
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;

        handlewin = 0; 

    if (debugLevel > 0)
        cerr << "RemoteVSTServer::showGUI(" << "): guiVisible is " << guiVisible << endl;

    if (haveGui == false)
    {
#ifdef WCLASS        
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
#ifdef TRACKTIONWM   
        UnregisterClassA(APPLICATION_CLASS_NAME2, GetModuleHandle(0));
#endif 
#endif          
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));     
        return;
    }

    if (guiVisible)
    {
#ifdef WCLASS        
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
#ifdef TRACKTIONWM   
        UnregisterClassA(APPLICATION_CLASS_NAME2, GetModuleHandle(0));
#endif 
#endif      
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));           
        return;
    }

    // if (hWnd)
   //     DestroyWindow(hWnd);
 //   hWnd = 0;
 
#ifdef EMBEDDRAG
    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_ACCEPTFILES, APPLICATION_CLASS_NAME, "LinVst", WS_POPUP, 0, 0, 200, 200, 0, 0, GetModuleHandle(0), 0);
#else
    hWnd = CreateWindowEx(WS_EX_TOOLWINDOW, APPLICATION_CLASS_NAME, "LinVst", WS_POPUP, 0, 0, 200, 200, 0, 0, GetModuleHandle(0), 0);
#endif
    if (!hWnd)
    {
        // cerr << "dssi-vst-server: ERROR: Failed to create window!\n" << endl;
#ifdef WCLASS 
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
#ifdef TRACKTIONWM   
        UnregisterClassA(APPLICATION_CLASS_NAME2, GetModuleHandle(0));
#endif 
#endif       
        guiVisible = false;
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));
        return;
    }
	
    rect = 0;
 
    vst2wrap->editor->getRect (&rect);
    vst2wrap->editor->open (hWnd);
    vst2wrap->editor->getRect (&rect);
    
    if (!rect)
    {
        // cerr << "dssi-vst-server: ERROR: Plugin failed to report window size\n" << endl;
        DestroyWindow(hWnd);
#ifdef WCLASS         
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
#ifdef TRACKTIONWM   
        UnregisterClassA(APPLICATION_CLASS_NAME2, GetModuleHandle(0));
#endif 
#endif             
        guiVisible = false;
        winm->handle = 0;
        winm->width = 0;
        winm->height = 0;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));
        return;
    }
#ifdef TRACKTIONWM
	    if(hosttracktion == 1)
        {
        RECT offsetcl, offsetwin;
        POINT offset;

        HWND hWnd2 = CreateWindow(APPLICATION_CLASS_NAME2, "LinVst", WS_CAPTION, 0, 0, 200, 200, 0, 0, GetModuleHandle(0), 0);
        GetClientRect(hWnd2, &offsetcl);
        GetWindowRect(hWnd2, &offsetwin);
        DestroyWindow(hWnd2);

        offset.x = (offsetwin.right - offsetwin.left) - offsetcl.right;
        offset.y = (offsetwin.bottom - offsetwin.top) - offsetcl.bottom;

        // if(GetSystemMetrics(SM_CMONITORS) > 1)	
        SetWindowPos(hWnd, HWND_TOP, GetSystemMetrics(SM_XVIRTUALSCREEN) + offset.x, GetSystemMetrics(SM_YVIRTUALSCREEN) + offset.y, rect->right - rect->left, rect->bottom - rect->top, 0); 
	// else
	// SetWindowPos(hWnd, HWND_TOP, offset.x, offset.y, rect->right - rect->left, rect->bottom - rect->top, 0);  
        }
	else
	{
        // if(GetSystemMetrics(SM_CMONITORS) > 1)
        SetWindowPos(hWnd, HWND_TOP, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), rect->right - rect->left, rect->bottom - rect->top, 0);
        // else
        // SetWindowPos(hWnd, HWND_TOP, 0, 0, rect->right - rect->left, rect->bottom - rect->top, 0);
	}
#else
        // if(GetSystemMetrics(SM_CMONITORS) > 1)
        SetWindowPos(hWnd, HWND_TOP, GetSystemMetrics(SM_XVIRTUALSCREEN), GetSystemMetrics(SM_YVIRTUALSCREEN), rect->right - rect->left, rect->bottom - rect->top, 0);
        // else
        // SetWindowPos(hWnd, HWND_TOP, 0, 0, rect->right - rect->left, rect->bottom - rect->top, 0);
#endif
        if (debugLevel > 0)
            cerr << "dssi-vst-server[1]: sized window" << endl;

        winm->width = rect->right - rect->left;
        winm->height = rect->bottom - rect->top;
        handlewin = GetPropA(hWnd, "__wine_x11_whole_window");
        winm->handle = (long int) handlewin;
        tryWrite(&m_shm[FIXED_SHM_SIZE], winm, sizeof(winmessage));
#else
    if (debugLevel > 0)
        cerr << "RemoteVSTServer::showGUI(" << "): guiVisible is " << guiVisible << endl;

    if (haveGui == false)
    {
#ifdef WCLASS     
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));  
#endif       
        guiVisible = false;
        return;
    }
 
    if (guiVisible)
    {
#ifdef WCLASS     
        UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
#endif          
        return;
    }
	
	#ifdef DRAG
        hWnd = CreateWindowEx(WS_EX_ACCEPTFILES, APPLICATION_CLASS_NAME, "LinVst", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);	    
	#else    
        hWnd = CreateWindow(APPLICATION_CLASS_NAME, "LinVst", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);
	#endif    
    if (!hWnd)
    {
#ifdef WCLASS     
    UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0)); 
#endif                
    cerr << "dssi-vst-server: ERROR: Failed to create window!\n" << endl;
    guiVisible = false;
    return;
    }

    if(hWnd)
    SetWindowText(hWnd, m_name.c_str());
	
    rect = 0;
    
    vst2wrap->editor->getRect (&rect);
    vst2wrap->editor->open (hWnd);
    vst2wrap->editor->getRect (&rect);
    
    if (!rect)
    {
#ifdef WCLASS     
    UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0)); 
#endif        
    cerr << "dssi-vst-server: ERROR: Plugin failed to report window size\n" << endl;
    guiVisible = false;
    return;
    }
    else
    {	    
#ifdef WINONTOP
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, rect->right - rect->left + 6, rect->bottom - rect->top + 25, SWP_NOMOVE);
#else	
        SetWindowPos(hWnd, HWND_TOP, 0, 0, rect->right - rect->left + 6, rect->bottom - rect->top + 25, SWP_NOMOVE);        
#endif
        if (debugLevel > 0)
            cerr << "dssi-vst-server[1]: sized window" << endl;

	guiVisible = true;
        ShowWindow(hWnd, SW_SHOWNORMAL);
        UpdateWindow(hWnd);
    }
#endif	
     //   timerval = 678;
     //   timerval = SetTimer(hWnd, timerval, 80, 0);
}
	
void RemoteVSTServer::hideGUI()
{
    // if (!hWnd)
        // return;
/*
    if ((haveGui == false) || (guiVisible == false))
    {
    hideguival = 0;
    return;
    }
    
*/    

#ifndef EMBED
  ShowWindow(hWnd, SW_HIDE);
  UpdateWindow(hWnd);
#endif
 
  if(melda == 0)
  {
  vst2wrap->editor->close ();	  	   	  	  
  }	  

#ifdef EMBED    
#ifndef WCLASS  
    if(remoteVSTServerInstance->haveGui == true)
    {
    if(hWnd)
    {
    HWND child = GetWindow(hWnd, GW_CHILD);  
    if(child)
    DestroyWindow(child);
    }
    }
#endif 
#endif   	      
		
  if(hWnd)
  {	
  // KillTimer(hWnd, timerval);
  #ifdef EMBED
  #ifdef XEMBED
  DestroyWindow(hWnd);
  #else
  #ifdef XECLOSE	  
  DestroyWindow(hWnd); 
  #endif	  
  #endif	  
  #else
  DestroyWindow(hWnd);
  #endif 
  #ifdef WCLASS  
  UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
  #ifdef EMBED        
  #ifdef TRACKTIONWM   
  UnregisterClassA(APPLICATION_CLASS_NAME2, GetModuleHandle(0));
  #endif  
  #endif 
  #endif         	  
  }
	
  if(melda == 1)
  {
  vst2wrap->editor->close ();	  	   	  	  
  }	  	
     	
  guiVisible = false;
	
  hidegui = 0;	

   // if (!exiting)
    //    usleep(50000);
}

#ifdef EMBED
void RemoteVSTServer::openGUI()
{
    guiVisible = true;
    ShowWindow(hWnd, SW_SHOWNORMAL);
    // ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);	
}
#endif

int RemoteVSTServer::processVstEvents()
{
    int         els;
    int         *ptr;
    int         sizeidx = 0;
    int         size;
    VstEvents   *evptr;

    ptr = (int *) m_shm2;
    els = *ptr;
    sizeidx = sizeof(int);

    if (els > VSTSIZE)
        els = VSTSIZE;

    evptr = &vstev[0];
    evptr->numEvents = els;
    evptr->reserved = 0;

    for (int i = 0; i < els; i++)
    {
        VstEvent* bsize = (VstEvent*) &m_shm2[sizeidx];
        size = bsize->byteSize + (2*sizeof(VstInt32));
        evptr->events[i] = bsize;
        sizeidx += size;
    }
  
    vst2wrap->processEvents (evptr);

    return 1;
}

void RemoteVSTServer::getChunk()
{
#ifdef CHUNKBUF
    int bnk_prg = readIntring(&m_shmControl5->ringBuffer);    
    int sz = vst2wrap->getChunk ((void**) &chunkptr, bnk_prg ? true : false);    
if(sz >= CHUNKSIZEMAX)
{
    writeInt(&m_shm[FIXED_SHM_SIZE], sz);
    return;
}
else
{
    if(sz < CHUNKSIZEMAX)    
    tryWrite(&m_shm[FIXED_SHM_SIZECHUNKSTART], chunkptr, sz);
    writeInt(&m_shm[FIXED_SHM_SIZE], sz);
    return;
}
#else
    void *ptr;
    int bnk_prg = readIntring(&m_shmControl5->ringBuffer);  
    int sz = vst2wrap->getChunk ((void**) &ptr, bnk_prg ? true : false);  
    if(sz < CHUNKSIZEMAX)
    tryWrite(&m_shm[FIXED_SHM_SIZECHUNKSTART], ptr, sz);
    writeInt(&m_shm[FIXED_SHM_SIZE], sz);
    return;
#endif
}

void RemoteVSTServer::setChunk()
{
#ifdef CHUNKBUF
    int sz = readIntring(&m_shmControl5->ringBuffer);
    if(sz >= CHUNKSIZEMAX)
{
    int bnk_prg = readIntring(&m_shmControl5->ringBuffer);
    void *ptr = chunkptr2;    
    int r = vst2wrap->setChunk (ptr, sz, bnk_prg ? true : false);    
    free(chunkptr2);
    writeInt(&m_shm[FIXED_SHM_SIZE], r);
    return;
}
else
{
    int bnk_prg = readIntring(&m_shmControl5->ringBuffer);
    void *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART];    
    int r = vst2wrap->setChunk (ptr, sz, bnk_prg ? true : false);      
    writeInt(&m_shm[FIXED_SHM_SIZE], r);
    return;
}
#else
    int sz = readIntring(&m_shmControl5->ringBuffer);
    int bnk_prg = readIntring(&m_shmControl5->ringBuffer);
    void *ptr = &m_shm[FIXED_SHM_SIZECHUNKSTART]; 
    int r = vst2wrap->setChunk (ptr, sz, bnk_prg ? true : false); 
    writeInt(&m_shm[FIXED_SHM_SIZE], r);
    return;
#endif
}

void RemoteVSTServer::canBeAutomated()
{
    int param = readIntring(&m_shmControl5->ringBuffer);
    int r = vst2wrap->canParameterBeAutomated (param);
    writeInt(&m_shm[FIXED_SHM_SIZE], r);
}

void RemoteVSTServer::getProgram()
{  
    int r = vst2wrap->getProgram(); 
    writeInt(&m_shm[FIXED_SHM_SIZE], r);
}

#ifdef SEM

void
RemoteVSTServer::waitForServer()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl->runServer);

    timespec ts_timeout;
    clock_gettime(CLOCK_REALTIME, &ts_timeout);
    ts_timeout.tv_sec += 60;
    if (sem_timedwait(&m_shmControl->runClient, &ts_timeout) != 0) {
         if(m_inexcept == 0)
         RemotePluginClosedException();
    }
    }
    else
    {
    fpost(&m_shmControl->runServer386);

    if (fwait(&m_shmControl->runClient386, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
   }
}

void
RemoteVSTServer::waitForServerexit()
{
    if(m_386run == 0)
    {
    sem_post(&m_shmControl->runServer);
    }
    else
    {
    fpost(&m_shmControl->runServer386);
   }
}

#else

void
RemoteVSTServer::waitForServer()
{
    fpost(&m_shmControl->runServer);

    if (fwait(&m_shmControl->runClient, 60000)) {
         if(m_inexcept == 0)
	 RemotePluginClosedException();
    }
}

void
RemoteVSTServer::waitForServerexit()
{
    fpost(&m_shmControl->runServer);
}

#endif

#ifdef VESTIGE
VstIntPtr VESTIGECALLBACK hostCallback(AEffect *plugin, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
#else
VstIntPtr VSTCALLBACK hostCallback(AEffect *plugin, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)
#endif
{
    VstIntPtr rv = 0;
    int retval = 0;
    
    switch (opcode)
    {
    case audioMasterAutomate:
    if(remoteVSTServerInstance)
    {	    
    if(!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {	    
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, index);
    remoteVSTServerInstance->writeFloatring(&remoteVSTServerInstance->m_shmControl->ringBuffer, opt);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
        break;

    case audioMasterVersion:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterVersion requested" << endl;
        rv = 2400;
        break;

    case audioMasterCurrentId:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterCurrentId requested" << endl;
#ifdef WAVES
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
    }
    }
#endif
        break;

    case audioMasterIdle:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterIdle requested " << endl;
        // plugin->dispatcher(plugin, effEditIdle, 0, 0, 0, 0);
        break;

    case audioMasterGetTime:
    
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun && remoteVSTServerInstance->m_shm3)
    {
#ifndef NEWTIME 	    
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, value);   
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
/*
    if(remoteVSTServerInstance->timeinfo)
    {
    memcpy(remoteVSTServerInstance->timeinfo, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], sizeof(VstTimeInfo));

  //   printf("%f\n", remoteVSTServerInstance->timeinfo->sampleRate);

    rv = (long)remoteVSTServerInstance->timeinfo;
    }
  */
   memcpy(&remoteVSTServerInstance->timeInfo, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], sizeof(VstTimeInfo));

  //   printf("%f\n", remoteVSTServerInstance->timeInfo.sampleRate);
 
  rv = (VstIntPtr)&remoteVSTServerInstance->timeInfo;	    	    
#else
    /*
    if(remoteVSTServerInstance->timeinfo)
    {
    memcpy(remoteVSTServerInstance->timeinfo, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], sizeof(VstTimeInfo));

 //    printf("%f\n", remoteVSTServerInstance->timeinfo->sampleRate);

    rv = (long)remoteVSTServerInstance->timeinfo;
    } 
    */
    memcpy(&remoteVSTServerInstance->timeInfo, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3 - sizeof(VstTimeInfo)], sizeof(VstTimeInfo));

  //   printf("%f\n", remoteVSTServerInstance->timeInfo.sampleRate);
  
    rv = (VstIntPtr)&remoteVSTServerInstance->timeInfo;   	    
#endif        
    }
    }
    
        break;

    case audioMasterProcessEvents:
            if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterProcessEvents requested" << endl;
        {
            VstEvents   *evnts;
            int         eventnum;
            int         *ptr2;
            int         sizeidx = 0;
            int         ok;

	    if(remoteVSTServerInstance)
            {	
            if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
            {
                evnts = (VstEvents*)ptr;

                if ((!evnts) || (evnts->numEvents <= 0))
                    break;

                eventnum = evnts->numEvents;

                ptr2 = (int *)remoteVSTServerInstance->m_shm3;

                sizeidx = sizeof(int);

                if (eventnum > VSTSIZE)
                    eventnum = VSTSIZE;            

                for (int i = 0; i < evnts->numEvents; i++)
                {

                    VstEvent *pEvent = evnts->events[i];
                    if (pEvent->type == kVstSysExType)
                        eventnum--;
                    else
                    {
                        unsigned int size = (2*sizeof(VstInt32)) + evnts->events[i]->byteSize;
                        memcpy(&remoteVSTServerInstance->m_shm3[sizeidx], evnts->events[i], size);
                        sizeidx += size;
                    }
                }
                *ptr2 = eventnum;

    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
  //  remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, value);
   
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
        }       
       }
      }
        break;

    case audioMasterIOChanged:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterIOChanged requested" << endl;
    {        
    struct amessage
    {
        int flags;
        int pcount;
        int parcount;
        int incount;
        int outcount;
        int delay;
    } am;

    if(remoteVSTServerInstance)
    {		    
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
     //   am.flags = plugin->flags;
        am.flags = remoteVSTServerInstance->mpluginptr->flags;
        am.pcount = plugin->numPrograms;
        am.parcount = plugin->numParams;
        am.incount = plugin->numInputs;
        am.outcount = plugin->numOutputs;
        am.delay = plugin->initialDelay;
#ifdef DOUBLEP
        if(remoteVSTServerInstance->vst2wrap->doublereplacing == true)
        am.flags |= effFlagsCanDoubleReplacing; 
        am.flags |= effFlagsCanReplacing; 
#else
        am.flags &= ~effFlagsCanDoubleReplacing;
        am.flags |= effFlagsCanReplacing; 
#endif        
        am.flags |= effFlagsCanReplacing; 

        memcpy(&remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], &am, sizeof(am));

    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
   
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
	    
    if((am.incount != remoteVSTServerInstance->m_numInputs) || (am.outcount != remoteVSTServerInstance->m_numOutputs))
    {
    if ((am.incount + am.outcount) * remoteVSTServerInstance->m_bufferSize * sizeof(float) < (PROCESSSIZE))
    {
    remoteVSTServerInstance->m_updateio = 1;
    remoteVSTServerInstance->m_updatein = am.incount;
    remoteVSTServerInstance->m_updateout = am.outcount;
    }
    }

      //  AEffect* update = remoteVSTServerInstance->m_plugin;
      //  update->flags = am.flags;
      //  update->numPrograms = am.pcount;
      //  update->numParams = am.parcount;
      //  update->numInputs = am.incount;
      //  update->numOutputs = am.outcount;
      //  update->initialDelay = am.delay;

       }
      }
     }
        break;

    case audioMasterSizeWindow:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterSizeWindow requested" << endl;
{
#ifdef EMBED
#ifndef TRACKTIONWM
#ifdef EMBEDRESIZE
   int opcodegui = 123456789;
	
    if(remoteVSTServerInstance)
    {	
    if (remoteVSTServerInstance->hWnd && remoteVSTServerInstance->guiVisible && !remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun && (remoteVSTServerInstance->guiupdate == 0))
    {	
    remoteVSTServerInstance->guiresizewidth = index;
    remoteVSTServerInstance->guiresizeheight = value;

    // ShowWindow(remoteVSTServerInstance->hWnd, SW_HIDE);
    // SetWindowPos(remoteVSTServerInstance->hWnd, HWND_TOP, 0, 0, remoteVSTServerInstance->guiresizewidth, remoteVSTServerInstance->guiresizeheight, 0);
	    
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcodegui);
    remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, index);
    remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, value);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    remoteVSTServerInstance->guiupdate = 1;
    rv = 1;
    }
    }
#endif
#endif
#else
     if(remoteVSTServerInstance)
     {	
     if (remoteVSTServerInstance->hWnd && !remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun && remoteVSTServerInstance->guiVisible)
     {
/*
//    SetWindowPos(remoteVSTServerInstance->hWnd, 0, 0, 0, index + 6, value + 25, SWP_NOMOVE | SWP_HIDEWINDOW);	
    SetWindowPos(remoteVSTServerInstance->hWnd, 0, 0, 0, index + 6, value + 25, SWP_NOMOVE);	
    ShowWindow(remoteVSTServerInstance->hWnd, SW_SHOWNORMAL);
    UpdateWindow(remoteVSTServerInstance->hWnd);
*/
    remoteVSTServerInstance->guiresizewidth = index;
    remoteVSTServerInstance->guiresizeheight = value;
    remoteVSTServerInstance->guiupdate = 1;	
    rv = 1;
        }
	}
#endif
}	    
        break;

    case audioMasterGetSampleRate:
          if (debugLevel > 1)
           cerr << "dssi-vst-server[2]: audioMasterGetSampleRate requested" << endl;
/*		
    if(remoteVSTServerInstance)
    {	
        if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
        {
        if (!remoteVSTServerInstance->sampleRate)
        {
            //  cerr << "WARNING: Sample rate requested but not yet set" << endl;
            break;
        }
        plugin->dispatcher(plugin, effSetSampleRate, 0, 0, NULL, (float)remoteVSTServerInstance->sampleRate);
        }
	}
*/	
/*   
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
    */
    if(remoteVSTServerInstance)
    {	    
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    rv = remoteVSTServerInstance->sampleRate;
    }
    }
        break;

    case audioMasterGetBlockSize:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetBlockSize requested" << endl;
/*
    if(remoteVSTServerInstance)
    {	
        if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
        {
        if (!remoteVSTServerInstance->bufferSize)
        {
            // cerr << "WARNING: Buffer size requested but not yet set" << endl;
            break;
        }
        plugin->dispatcher(plugin, effSetBlockSize, 0, remoteVSTServerInstance->bufferSize, NULL, 0);
        }
	}
*/	
/*
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
 */
    if(remoteVSTServerInstance)
    {
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    rv = remoteVSTServerInstance->bufferSize;
    }
    }
        break;

    case audioMasterGetInputLatency:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetInputLatency requested" << endl;
/*		    
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
*/
        break;

    case audioMasterGetOutputLatency:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetOutputLatency requested" << endl;
/*		    
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
*/
        break;

    case audioMasterGetCurrentProcessLevel:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetCurrentProcessLevel requested" << endl;
        // 0 -> unsupported, 1 -> gui, 2 -> process, 3 -> midi/timer, 4 -> offline
		    
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
        if (remoteVSTServerInstance->inProcessThread)
            rv = 2;
        else
            rv = 1;
    }
    }
        break;

    case audioMasterGetAutomationState:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetAutomationState requested" << endl;

    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
   //     rv = 4; // read/write
        break;

    case audioMasterOfflineStart:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterOfflineStart requested" << endl;
        break;

    case audioMasterOfflineRead:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterOfflineRead requested" << endl;
        break;

    case audioMasterOfflineWrite:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterOfflineWrite requested" << endl;
        break;

    case audioMasterOfflineGetCurrentPass:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterOfflineGetCurrentPass requested" << endl;
        break;

    case audioMasterOfflineGetCurrentMetaPass:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterOfflineGetCurrentMetaPass requested" << endl;
        break;

    case audioMasterGetVendorString:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetVendorString requested" << endl;
{
    char retstr[512];

    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    strcpy(retstr, remoteVSTServerInstance->readString(&remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3]).c_str()); 
    strcpy((char *)ptr, retstr);
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3 + 512], sizeof(int));
    rv = retval;
      }
     }
   }
        break;

    case audioMasterGetProductString:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetProductString requested" << endl;
{
    char retstr[512];

    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    strcpy(retstr, remoteVSTServerInstance->readString(&remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3]).c_str()); 
    strcpy((char *)ptr, retstr);
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3 + 512], sizeof(int));
    rv = retval;
      }
     }
    }
        break;

    case audioMasterGetVendorVersion:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetVendorVersion requested" << endl;
  
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
    }
    }	    
        break;

    case audioMasterVendorSpecific:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterVendorSpecific requested" << endl;
        break;

    case audioMasterCanDo:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterCanDo(" << (char *)ptr << ") requested" << endl;
#ifdef CANDOEFF
    {
    int retval;

    if(remoteVSTServerInstance && !remoteVSTServerInstance->exiting)
    {	
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance-> writeString(&remoteVSTServerInstance->m_shm[FIXED_SHM_SIZE3], (char*)ptr);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();

    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;  
    }
    }
#else		    
        if (!strcmp((char*)ptr, "sendVstEvents")
                    || !strcmp((char*)ptr, "sendVstMidiEvent")
                    || !strcmp((char*)ptr, "sendVstMidiEvent")
                    || !strcmp((char*)ptr, "receiveVstEvents")
                    || !strcmp((char*)ptr, "receiveVstMidiEvents")
                    || !strcmp((char*)ptr, "receiveVstTimeInfo")
#ifdef WAVES
                    || !strcmp((char*)ptr, "shellCategory")
                    || !strcmp((char*)ptr, "supportShell")
#endif
                    || !strcmp((char*)ptr, "acceptIOChanges")
                    || !strcmp((char*)ptr, "startStopProcess")
#ifdef EMBED
#ifdef EMBEDRESIZE
                    || !strcmp((char*)ptr, "sizeWindow")
#endif
#else
                    || !strcmp((char*)ptr, "sizeWindow")
#endif
                    // || !strcmp((char*)ptr, "supplyIdle")
                    )
            rv = 1;
#endif 		    
        break;

    case audioMasterGetLanguage:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetLanguage requested" << endl;
        rv = kVstLangEnglish;
        break;

    case audioMasterGetDirectory:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterGetDirectory requested" << endl;
        break;

    case audioMasterUpdateDisplay:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterUpdateDisplay requested" << endl;
        break;

    case audioMasterBeginEdit:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterBeginEdit requested" << endl;
    
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, index);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }	    
        break;

    case audioMasterEndEdit:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterEndEdit requested" << endl;
    
    if(remoteVSTServerInstance)
    {	
    if (!remoteVSTServerInstance->exiting && remoteVSTServerInstance->effectrun)
    {
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)opcode);
    remoteVSTServerInstance->writeIntring(&remoteVSTServerInstance->m_shmControl->ringBuffer, index);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();
    retval = 0;
    memcpy(&retval, &remoteVSTServerInstance->m_shm3[FIXED_SHM_SIZE3], sizeof(int));
    rv = retval;
      }
     }
        break;

    case audioMasterOpenFileSelector:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterOpenFileSelector requested" << endl;
        break;

    case audioMasterCloseFileSelector:
        if (debugLevel > 1)
            cerr << "dssi-vst-server[2]: audioMasterCloseFileSelector requested" << endl;
        break;

    default:
        if (debugLevel > 0)
            cerr << "dssi-vst-server[0]: unsupported audioMaster callback opcode " << opcode << endl;
	break;	    
    }
    return rv;
}

VOID CALLBACK TimerProc(HWND hWnd, UINT message, UINT idTimer, DWORD dwTime)
{
   HWND hwnderr = FindWindow(NULL, "LinVst Error");
   SendMessage(hwnderr, WM_COMMAND, IDCANCEL, 0);
}

void RemoteVSTServer::guiUpdate()
{
#ifdef EMBED
#ifdef EMBEDRESIZE
   remoteVSTServerInstance->guiupdatecount += 1;
	
   if(remoteVSTServerInstance->guiupdatecount == 2)
   {
   ShowWindow(remoteVSTServerInstance->hWnd, SW_SHOWNORMAL);
   UpdateWindow(remoteVSTServerInstance->hWnd);
   remoteVSTServerInstance->guiupdate = 0;
   remoteVSTServerInstance->guiupdatecount = 0;
   }
#endif
#endif
#ifndef EMBED
//      SetWindowPos(remoteVSTServerInstance->hWnd, 0, 0, 0, remoteVSTServerInstance->guiresizewidth + 6, remoteVSTServerInstance->guiresizeheight + 25, SWP_NOMOVE | SWP_HIDEWINDOW);	
    SetWindowPos(remoteVSTServerInstance->hWnd, 0, 0, 0, remoteVSTServerInstance->guiresizewidth + 6, remoteVSTServerInstance->guiresizeheight + 25, SWP_NOMOVE);	
    ShowWindow(remoteVSTServerInstance->hWnd, SW_SHOWNORMAL);
    UpdateWindow(remoteVSTServerInstance->hWnd);
    remoteVSTServerInstance->guiupdate = 0;
#endif
}

bool InitModule ()   
{
	return true; 
}

bool DeinitModule ()
{
	return true; 
}

#define mchr(a,b,c,d) ( ((a)<<24) | ((b)<<16) | ((c)<<8) | (d) )

Steinberg::Vst::Vst2Wrapper *createEffectInstance2(audioMasterCallback audioMaster, HMODULE libHandle, std::string libnamepath, std::string partidx, Steinberg::IPluginFactory* factory, int *vstuid)
{  
std::string libnamepath2;
bool        test;
Steinberg::FIDString cid;
Steinberg::FIDString firstcid;
int audioclasscount = 0;
char ibuf[256];
int parthit = 0;
int parthit2 = 0;
int audioclassfirst = 0;
int audioclass = 0;
int firstdone = 0;

    if(libnamepath.find(".vst3") != std::string::npos)
    {
    libnamepath.replace(libnamepath.begin() + libnamepath.find(".vst3"), libnamepath.end(), "");
    }
    else if(libnamepath.find(".Vst3") != std::string::npos)
    {
    libnamepath.replace(libnamepath.begin() + libnamepath.find(".Vst3"), libnamepath.end(), "");
    }
    else if(libnamepath.find(".VST3") != std::string::npos)
    {
    libnamepath.replace(libnamepath.begin() + libnamepath.find(".VST3"), libnamepath.end(), "");
    }
         
    auto proc = (GetFactoryProc)::GetProcAddress ((HMODULE)libHandle, "GetPluginFactory");
	
    if(!proc)
    return nullptr;
  
    factory = proc();
    
    if(!factory)
    return nullptr;
        
    Steinberg::PFactoryInfo factoryInfo;
    
    factory->getFactoryInfo (&factoryInfo);
    	
	// cout << "  Factory Info:\n\tvendor = " << factoryInfo.vendor << "\n\turl = " << factoryInfo.url << "\n\temail = " << factoryInfo.email << "\n\n" << endl;
		
    int countclasses = factory->countClasses ();
   
    if(countclasses == 0)
    return nullptr;
    
    int idval = 0;
		   
    for (Steinberg::int32 i = 0; i < countclasses; i++)			
	{	
	Steinberg::PClassInfo classInfo;	
	
	factory->getClassInfo (i, &classInfo);
						
    if (strcmp(classInfo.category, "Audio Module Class") == 0)
    {     
    if(audioclassfirst == 0 && firstdone == 0)
    {
    audioclassfirst = i;
    firstdone = 1;
    }
                        
    audioclass = i;
    
    memset(ibuf, 0, sizeof(ibuf));
        
    if(i < 10)
    {
    sprintf(ibuf,"%d",0);
    sprintf(&ibuf[1],"%d", i);   
    }
    else    
    sprintf(ibuf,"%d", i);
    
    libnamepath2 = libnamepath + "-" + "part" + "-" + ibuf + ".so";
	 
	test = std::ifstream(libnamepath2.c_str()).good();  
     
    if((atoi(partidx.c_str()) == (10000)) && !test && (audioclasscount > 0))
    {
    parthit = 1;
         
    std::ifstream source(libnamepath + ".so", std::ios::binary);
         
    std::ofstream dest(libnamepath2, std::ios::binary);
     
    dest << source.rdbuf();
         
    source.close();
    dest.close();
    }
    else if((atoi(partidx.c_str()) == (i)) && test && (audioclasscount > 0))
    {
    parthit2 = 1;
    break;
    }
    
    audioclasscount++;
             
	} // Audio Module Class	
    } // for
 
    if((atoi(partidx.c_str()) == (10000)) && (parthit == 1))
    audioclass = audioclassfirst;
        
    if((atoi(partidx.c_str()) == (10000)) && (parthit == 0 && parthit2 == 0))
    audioclass = audioclassfirst;  
    
    Steinberg::PClassInfo classInfo2;     

    factory->getClassInfo (audioclass, &classInfo2);
    cid = classInfo2.cid;
        
    Steinberg::FIDString iid = Steinberg::Vst::IComponent::iid;       
       
    Steinberg::char8 cidString[50];
	Steinberg::FUID (classInfo2.cid).toRegistryString (cidString);
	Steinberg::String cidStr (cidString);
	// cout << "  Class Info " << audioclass << ":\n\tname = " << classInfo2.name << "\n\tcategory = " << classInfo2.category << "\n\tcid = " << cidStr.text8 () << "\n\n" << endl;	 
	
	idval = mchr(cidStr[1], cidStr[10], cidStr[15], cidStr[20]);  
	
	int idval1 = cidStr[1] + cidStr[2] + cidStr[3] + cidStr[4];
    int idval2 = cidStr[5] + cidStr[6] + cidStr[7]+ cidStr[8];
    int idval3 = cidStr[10] + cidStr[11] + cidStr[12] + cidStr[13];
    int idval4 = cidStr[15] + cidStr[16] + cidStr[17] + cidStr[18];
    int idval5 = cidStr[20] + cidStr[21] + cidStr[22] + cidStr[23];
    int idval6 = cidStr[25] + cidStr[26] + cidStr[27] + cidStr[28];
    int idval7 = cidStr[29] + cidStr[30] + cidStr[31] + cidStr[32];
    int idval8 = cidStr[33] + cidStr[34] + cidStr[35] + cidStr[36];

    int idvalout = 0;

    idvalout += idval1;
    idvalout += idval2 << 1;
    idvalout += idval3 << 2;
    idvalout += idval4 << 3;
    idvalout += idval5 << 4;
    idvalout += idval6 << 5;
    idvalout += idval7 << 6;
    idvalout += idval8 << 7;

    idval += idvalout;
	
	*vstuid = idval;  

    return Steinberg::Vst::Vst2Wrapper::create(factory, cid, idval, audioMaster);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow) 
{
    HANDLE  ThreadHandle[3] = {0,0,0};
    string pathName;
    string fileName;
    char cdpath[4096];
    char *libname = 0;
    char *libname2 = 0;
    char *fileInfo = 0;
    string libnamepath;
    string partidx;
    char *outname[10];
    int offset = 0;
    int idx = 0;
    int ci = 0;
            
    cout << "DSSI VST plugin server v" << RemotePluginVersion << endl;
    cout << "Copyright (c) 2012-2013 Filipe Coelho" << endl;
    cout << "Copyright (c) 2010-2011 Kristian Amlie" << endl;
    cout << "Copyright (c) 2004-2006 Chris Cannam" << endl;
    cout << "LinVst3 version 1.62" << endl;
    
    /*

    if (cmdline)
    {
    int offset = 0;
    if (cmdline[0] == '"' || cmdline[0] == '\'') offset = 1;
    for (int ci = offset; cmdline[ci]; ++ci)
    {
    if (cmdline[ci] == ',')
    {
    libname2 = strndup(cmdline + offset, ci - offset);
    ++ci;
    if (cmdline[ci])
    {
    fileInfo = strdup(cmdline + ci);
    int l = strlen(fileInfo);
    if (fileInfo[l-1] == '"' || fileInfo[l-1] == '\'')
    fileInfo[l-1] = '\0';
    }
    }
    }
    }
    */
	
    if(cmdline[0] == '\0')
    {
    exit(0);
   // return 1;
    }	
    
    if(cmdline)
    { 
    if (cmdline[0] == '"' || cmdline[0] == '\'') offset = 1;                 
    for(ci = offset; cmdline[ci]; ++ci)
    {
    if(cmdline[ci] == ',')
    {
    outname[idx] = strndup(cmdline + offset, ci - offset);
    ++idx;
    ++ci;                    
    offset = ci;
    }   
    }
    outname[idx] = strndup(cmdline + offset, ci - offset);
    }
     
    partidx =  outname[0];   
    libname2 = outname[1];
    fileInfo = outname[2];
    int l = strlen(fileInfo);
    if (fileInfo[l-1] == '"' || fileInfo[l-1] == '\'')
    fileInfo[l-1] = '\0';
    
    // printf("fileinfo %s\n", fileInfo);    

    if (libname2 != NULL)
    {
    if ((libname2[0] == '/') && (libname2[1] == '/'))
    libname = strdup(&libname2[1]);
    else
    libname = strdup(libname2);
    }
    else
    {
    cerr << "Usage: dssi-vst-server <vstname.dll>,<tmpfilebase>" << endl;
    cerr << "(Command line was: " << cmdline << ")" << endl;
	    
	exit(0);    
    // return 1;
    }

    if (!libname || !libname[0] || !fileInfo || !fileInfo[0])
    {
    cerr << "Usage: dssi-vst-server <vstname.dll>,<tmpfilebase>" << endl;
    cerr << "(Command line was: " << cmdline << ")" << endl;
	    
	exit(0);    
    // return 1;
    }
	
    strcpy(cdpath, libname);
    pathName = cdpath;
    fileName = cdpath;
    size_t found = pathName.find_last_of("/");
    pathName = pathName.substr(0, found);
    size_t found2 = fileName.find_last_of("/");
    fileName = fileName.substr(found2 + 1, strlen(libname) - (found2 +1));
    SetCurrentDirectory(pathName.c_str());
    
    
    libnamepath = libname;

    cout << "Loading  " << libname << endl;

    HMODULE libHandle = ::LoadLibraryA(libname);

    if (!libHandle)
    {
#ifdef  VST6432
/*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Error loading plugin dll %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);    
    */
    cerr << "dssi-vst-server: ERROR: Couldn't load VST DLL \"" << libname << "\"" << endl;
    usleep(5000000);     	    
    exit(0);    
    // return 1;
#else
#ifdef VST32  
/*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Error loading plugin dll %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc); 
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer); 
    */ 
    cerr << "dssi-vst-server: ERROR: Couldn't load VST DLL \"" << libname << "\"" << endl;
    usleep(5000000);        	    
    exit(0);    
    // return 1;
#else
/*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Error loading plugin dll %s. This LinVst version is for 64 bit vst's only", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);
    cerr << "dssi-vst-server: ERROR: Couldn't load VST DLL \"" << fileName << "\" This LinVst version is for 64 bit vsts only." << endl;
*/  
    usleep(5000000);            
    exit(0);    
    // return 1;
#endif
#endif    
    }     
    
    InitModuleProc initProc = (InitModuleProc)::GetProcAddress ((HMODULE)libHandle, kInitModuleProcName);
	if (initProc)
	{
	if (initProc () == false)
	{
	cerr << "initprocerr" << endl;
	
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }
    usleep(5000000);         
	exit(0);
	// return 1; 
	}
    }       
	
	remoteVSTServerInstance = 0;
	
    string deviceName = fileName;
    size_t foundext = deviceName.find_last_of(".");
    deviceName = deviceName.substr(0, foundext);
    
    remoteVSTServerInstance = new RemoteVSTServer(fileInfo, deviceName);
	
    // remoteVSTServerInstance = new RemoteVSTServer(fileInfo, libname);
    
    if(!remoteVSTServerInstance)
    {
    cerr << "ERROR: Remote VST startup failed" << endl;
    /*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Remote VST startup failed %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer); 
    */ 
    usleep(5000000);      
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }   	 	
    exit(0);	
    // return 1; 
    }
        
    if(remoteVSTServerInstance->starterror == 1)
    {
    cerr << "ERROR: Remote VST startup failed and/or mismatched LinVst versions" << endl;
    /*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Remote VST startup failed and/or mismatched LinVst versions %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);   
    */ 
    usleep(5000000);     
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }		
    exit(0);	
    // return 1; 
    }
                
    audioMasterCallback hostCallbackFuncPtr = hostCallback;
               
    remoteVSTServerInstance->vst2wrap = createEffectInstance2 (hostCallbackFuncPtr, libHandle, libnamepath, partidx, remoteVSTServerInstance->factory, &remoteVSTServerInstance->vst2uid);
    
	if (!remoteVSTServerInstance->vst2wrap)
	{
    cerr << "dssi-vst-server: ERROR: Failed to instantiate plugin in VST DLL \"" << libname << "\"" << endl;
    /*
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Error getting instance %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);    
    */
    usleep(5000000);     
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }	    
    exit(0);
	// return 1;
    }
  
	// Return the VST AEffect structure
//	remoteVSTServerInstance->m_plugin = effect->getAeffect();
    
    /*    
    if (!remoteVSTServerInstance->m_plugin)
    {
    cerr << "dssi-vst-server: ERROR: Failed to instantiate plugin in VST DLL \"" << libname << "\"" << endl;
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Error getting instance %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);    
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }
	    
    exit(0);
	// return 1;
    }
    
    */
    
    /*

    if (remoteVSTServerInstance->m_plugin->magic != kEffectMagic)
    {
        cerr << "dssi-vst-server: ERROR: Not a VST plugin in DLL \"" << libname << "\"" << endl;
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Not a VST2 plugin DLL %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }
	    
	exit(0);    
        // return 1;
    }
    
    */
    
    /*

    if (!(remoteVSTServerInstance->m_plugin->flags & effFlagsCanReplacing))
    {
        cerr << "dssi-vst-server: ERROR: Plugin does not support processReplacing (required)" << endl;
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    TCHAR wbuf[1024];
    wsprintf(wbuf, "ProcessReplacing not supported %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer); 
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }
	    
	exit(0);    
        // return 1;
    }
    
    */

 //   if(remoteVSTServerInstance->m_plugin->flags & effFlagsHasEditor)
  
    if(remoteVSTServerInstance->vst2wrap->editor)
    {
    remoteVSTServerInstance->haveGui = true;
    remoteVSTServerInstance->mpluginptr->flags |= effFlagsHasEditor;    
    }
    else 
    remoteVSTServerInstance->haveGui = false;
	
    DWORD threadIdp = 0;
    ThreadHandle[0] = CreateThread(0, 0, AudioThreadMain, 0, 0, &threadIdp);
    if (!ThreadHandle[0])
    {
    cerr << "Failed to create audio thread!" << endl;
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    /*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Thread Error audio %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer); 
    */
    usleep(5000000); 
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }	    
	exit(0);    
        // return 1;
    }

    DWORD threadIdp2 = 0;
    ThreadHandle[1] = CreateThread(0, 0, GetSetThreadMain, 0, 0, &threadIdp2);
    if (!ThreadHandle[1])
    {
    cerr << "Failed to create getset thread!" << endl;
    
    remoteVSTServerInstance->exiting = 1;    
    sleep(1);  
    WaitForMultipleObjects(1, ThreadHandle, TRUE, 5000);    
    if (ThreadHandle[0])
    {
        // TerminateThread(ThreadHandle[0], 0);
        CloseHandle(ThreadHandle[0]);
    }    
//    TerminateThread(ThreadHandle[0], 0);

    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    /*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Thread Error getset %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);  
    */
    usleep(5000000);     
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }	    
	exit(0);    
        // return 1;
    }

    DWORD threadIdp3 = 0;
    ThreadHandle[2] = CreateThread(0, 0, ParThreadMain, 0, 0, &threadIdp3);
    if (!ThreadHandle[2])
    {
    cerr << "Failed to create par thread!" << endl;
    
    remoteVSTServerInstance->exiting = 1;    
    sleep(1);  
    WaitForMultipleObjects(2, ThreadHandle, TRUE, 5000);    
    if (ThreadHandle[0])
    {
        // TerminateThread(ThreadHandle[0], 0);
        CloseHandle(ThreadHandle[0]);
    }
    if (ThreadHandle[1])
    {
        // TerminateThread(ThreadHandle[1], 0);
        CloseHandle(ThreadHandle[1]);
    }    
//    TerminateThread(ThreadHandle[0], 0);
//    TerminateThread(ThreadHandle[1], 0);
    
    remoteVSTServerInstance->writeOpcodering(&remoteVSTServerInstance->m_shmControl->ringBuffer, (RemotePluginOpcode)disconnectserver);
    remoteVSTServerInstance->commitWrite(&remoteVSTServerInstance->m_shmControl->ringBuffer);
    remoteVSTServerInstance->waitForServer();  
    remoteVSTServerInstance->waitForClient2exit();
    remoteVSTServerInstance->waitForClient3exit();
    remoteVSTServerInstance->waitForClient4exit();
    remoteVSTServerInstance->waitForClient5exit();
    /*
    TCHAR wbuf[1024];
    wsprintf(wbuf, "Thread Error par %s", fileName.c_str());
    UINT_PTR errtimer = SetTimer(NULL, 800, 10000, (TIMERPROC) TimerProc);
    MessageBox(NULL, wbuf, "LinVst Error", MB_OK | MB_TOPMOST);
    KillTimer(NULL, errtimer);    
    */
    usleep(5000000);     
	if(remoteVSTServerInstance)
	delete remoteVSTServerInstance;
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }	    
	exit(0);    
        // return 1;
    }

    MSG msg;
    int tcount = 0;

    while (!remoteVSTServerInstance->exiting)
    {		         
    while(PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {       
    if(remoteVSTServerInstance->exiting)
    break;
	    
    if(msg.message == 15 && !remoteVSTServerInstance->guiVisible)
    break;    	    
    
    TranslateMessage(&msg);
    DispatchMessage(&msg);
       
    if(remoteVSTServerInstance->hidegui == 1)
    break;    
    } 
    
    if(remoteVSTServerInstance->hidegui == 1)
    {
    remoteVSTServerInstance->hideGUI();
    }
    
    if(remoteVSTServerInstance->exiting)
    break;	                   
    remoteVSTServerInstance->dispatchControl(50);               
    }
	
/*
    for (int i=0;i<10000;i++)
    {
        if (remoteVSTServerInstance->parfin && remoteVSTServerInstance->audfin && remoteVSTServerInstance->getfin)
            break;
	usleep(1000);    
    }
*/
	
    WaitForMultipleObjects(3, ThreadHandle, TRUE, 5000);

    if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: cleaning up" << endl;

    if (ThreadHandle[0])
    {
        // TerminateThread(ThreadHandle[0], 0);
        CloseHandle(ThreadHandle[0]);
    }

    if (ThreadHandle[1])
    {
        // TerminateThread(ThreadHandle[1], 0);
        CloseHandle(ThreadHandle[1]);
    }

    if (ThreadHandle[2])
    {
        // TerminateThread(ThreadHandle[2], 0);
        CloseHandle(ThreadHandle[2]);
    }

    if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: closed threads" << endl;
        
    if(remoteVSTServerInstance)
    delete remoteVSTServerInstance;
    	
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }

    if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: freed dll" << endl;
 //   if (debugLevel > 0)
        cerr << "dssi-vst-server[1]: exiting" << endl;
	
    exit(0);
	
 //   return 0;
}
