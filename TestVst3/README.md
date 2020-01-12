# TestVst3

TestVst3 can roughly test how a vst3 plugin might run under Wine.

It tries to open the vst3 and display the vst3 window for roughly 8 seconds and then closes.

Usage is ./testvst3.exe "vst3file.vst3"

paths and vst3 filenames that contain spaces need to be enclosed in quotes.

testvst3.exe is for 64 bit vst3's only

If TestVst3 is run from a terminal and the output is looked at, then sometimes there can be an unimplemented function error in some dll and that dll can then be overriden.

To make

This TestVst3 source folder needs to be placed within the VST3 SDK main folder (the VST3_SDK folder or the VST3 folder that contains the base, public.sdk, pluginterfaces etc folders) ie the TestVst3 source folder needs to be placed alongside the base, public.sdk, pluginterfaces etc folders of the VST3 SDK.

Then change into the TestVst3 folder and run make.
