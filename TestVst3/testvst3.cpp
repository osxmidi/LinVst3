#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include <string>

#define __cdecl
//#endif

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


#define WIN32_LEAN_AND_MEAN

#include <windows.h>

#include <shellapi.h>

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

#define OLD_PLUGIN_ENTRY_POINT "main"
#define NEW_PLUGIN_ENTRY_POINT "VSTPluginMain"

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

#ifdef VESTIGE
typedef AEffect *(VESTIGECALLBACK *VstEntry)(audioMasterCallback audioMaster);
#else
typedef AEffect *(VSTCALLBACK *VstEntry)(audioMasterCallback audioMaster);
#endif

using namespace std;

VstIntPtr VESTIGECALLBACK hostCallback(AEffect *plugin, VstInt32 opcode, VstInt32 index, VstIntPtr value, void *ptr, float opt)    
{		 
VstIntPtr rv = 0;
    
     switch (opcode)
     {  
     case audioMasterVersion:
     rv = 2400;
     break;
        
     default:
     break;           
     }    
    
     return rv; 
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

Steinberg::Vst::Vst2Wrapper *createEffectInstance2(audioMasterCallback audioMaster, HMODULE libHandle, Steinberg::IPluginFactory* factory, int *vstuid)
{  
bool        test;
Steinberg::FIDString cid;

int audioclassfirst = 0;
int audioclass = 0;
int firstdone = 0;
         
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
             
	} // Audio Module Class	
    } // for
 
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

std::string getMaker(Steinberg::Vst::Vst2Wrapper *vst2wrap)
{
      char buffer[512];
      memset(buffer, 0, sizeof(buffer));
      vst2wrap->getVendorString (buffer);
      if (buffer[0])
      return buffer;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdline, int cmdshow)
{ 
LPWSTR *args;
int numargs;

    Steinberg::Vst::Vst2Wrapper *vst2wrap;
    int vst2uid;
    Steinberg::IPluginFactory* factory;
   
    cout << "LinVst Vst Test" << endl;	

    args = CommandLineToArgvW(GetCommandLineW(), &numargs);
   
    if(args == NULL)
    {
    printf("CommandLine arguments failed\n");
    exit(0);
    }
    
    if(args[1] == NULL)
    {
	printf("Usage: vstest.exe path_to_vst_dll\n");	
	exit(0);
	}	
  
    HINSTANCE libHandle = 0;
    libHandle = LoadLibraryW(args[1]);
    if (!libHandle)
    {
	printf("LibraryLoadError\n");	
	exit(0);
	}	  
	
    LocalFree(args);
	
    InitModuleProc initProc = (InitModuleProc)::GetProcAddress ((HMODULE)libHandle, kInitModuleProcName);
	if (initProc)
	{
	if (initProc () == false)
	{
	cerr << "InitProcError" << endl;
	
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }        
	exit(0); 
	}
    }       

    audioMasterCallback hostCallbackFuncPtr = hostCallback;
               
    vst2wrap = createEffectInstance2 (hostCallbackFuncPtr, libHandle, factory, &vst2uid);
    
    if (!vst2wrap)
    {
    printf("InstnceError\n");
    
    if(libHandle)
    {
    ExitModuleProc exitProc = (ExitModuleProc)::GetProcAddress ((HMODULE)libHandle, kExitModuleProcName);
	if (exitProc)
	exitProc ();
    FreeLibrary(libHandle);	
    }        
	exit(0);         
    }
    
    if(vst2wrap->editor)
    {
    printf("HasEditor\n");  
    }
    else 
    {
    printf("NoEditor\n");
    
    vst2wrap->suspend ();
    
    if(vst2wrap)
    delete vst2wrap;  
	
    if (factory)
    factory->release ();   
      	
    if(libHandle)
    FreeLibrary(libHandle);  	
	exit(0);    
    
    }
	   	
	printf("NumInputs %d\n", vst2wrap->numinputs);
    printf("NumOutputs %d\n", vst2wrap->numoutputs);
		
	cout << "Maker " << getMaker(vst2wrap)	<< endl;	
	
	WNDCLASSEX          wclass;
		
    memset(&wclass, 0, sizeof(WNDCLASSEX));
    wclass.cbSize = sizeof(WNDCLASSEX);
    wclass.style = 0;
	// CS_HREDRAW | CS_VREDRAW;
    wclass.lpfnWndProc = DefWindowProc;
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
    vst2wrap->suspend ();
    
    if(vst2wrap)
    delete vst2wrap;  
	
    if (factory)
    factory->release ();   
      	
    if(libHandle)
    FreeLibrary(libHandle);  	
	exit(0);
	}

    HWND hWnd = CreateWindow(APPLICATION_CLASS_NAME, "LinVst", WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, GetModuleHandle(0), 0);
	    
    if (!hWnd)
    {
	UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
	
    vst2wrap->suspend ();
    
    if(vst2wrap)
    delete vst2wrap;  
	
    if (factory)
    factory->release ();    
		
    if(libHandle)
    FreeLibrary(libHandle);  	
	exit(0);		
	}
	
    ERect *rect = 0;
    vst2wrap->editor->getRect (&rect);
    vst2wrap->editor->open (hWnd);
    vst2wrap->editor->getRect (&rect);
    if (!rect)
    {	
	DestroyWindow(hWnd);	
	UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
    vst2wrap->suspend ();
    
    if(vst2wrap)
    delete vst2wrap;  
	
    if (factory)
    factory->release ();     	
    if(libHandle)
    FreeLibrary(libHandle);  	
	exit(0);
    }
	
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, rect->right - rect->left + 6, rect->bottom - rect->top + 25, SWP_NOMOVE);	
	    
    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);
        
    UINT_PTR timerval;
	    
    timerval = 678;
    timerval = SetTimer(hWnd, timerval, 80, 0);	
        
    int count = 0; 
	
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
        
    if((msg.message == WM_TIMER) && (msg.wParam == 678)) 
    {
    count += 1;
    
    if(count == 100)
    break;
    }              
    }	
    
    vst2wrap->editor->close ();
     
	DestroyWindow(hWnd);	
	UnregisterClassA(APPLICATION_CLASS_NAME, GetModuleHandle(0));
	        
    vst2wrap->suspend ();
    
    if(vst2wrap)
    delete vst2wrap;  
	
    if (factory)
    factory->release ();    
	
    if(libHandle)
    FreeLibrary(libHandle);    	  	
}

