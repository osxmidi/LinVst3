## Tested Vst3 Plugins

**Audio/Midi clips**
LinVst has a Drag-and-Drop version (see the dragwin folder for the Makefile).
The Audacity Windows version can be used to drag audio/midi clips from the vst window to Audacity (after maybe previewing/arranging them in the vst such as EZdrummer) and then there is the option of editing them in Audacity before saving and dragging them to the Linux Daw.
Drag the clip from the plugin window to Audacity, then select the track in Audacity and export it, then drag it to the Daw.

---------

https://github.com/osxmidi/LinVst/blob/master/Tested-VST-Plugins/README.md

**Kontakt**, see the above link for updated install tips regarding Native Access etc.

LinVst3 can run the Kontakt 7 etc vst3 version, tested using Wine-tkg https://github.com/osxmidi/LinVst/tree/master/Wine-tkg and a DirectX 11 capable card.

**EZDrummer3** 

EZDrummer3 drag/drop works with the LinVst3 drag/drop version.

**ModoDrum**
Midi Grooves are in ~/.wine/drive_c/Program Files/IK Multimedia/MODO DRUM
Dragging grooves to Daw tracks works with the LinVst3 drag and drop version if the Daw supports it.

**Valhalla** 

**Amplitube 5**

**Line 6 Helix Native** (maybe needed, msvcr120.dll and gdiplus.dll overrides or winetricks vcrun2013 gdiplus wininet) (copy and paste username and password into the registration window)

**Melda** 
Installer might need corefonts (winetricks corefonts)
Melda MXXX Multi Effects** (turn GPU acceleration off)

Melda drag/drop works with the LinVst3 drag/drop version.

**Neural Amp Modeler (NAM)** 

**Blue Cat** 

**Voxengo** 

**FabFilter** 

**Wave Arts**

**Mercuriall Spark Amp Sim**

**Amped Roots**

-------

**Hyperthreading**

For Reaper, in Options/Preferences/Buffering uncheck Auto-detect the number of needed audio processing threads and set 
Audio reading/processing threads to the amount of physical cores of the cpu (not virtual cores such as hyperthreading cores).

This can help with stutters and rough audio response.

Other Daws might have similar settings.

**Waveform**

For Waveform, disable sandbox option for plugins.

**Bitwig**

For Bitwig, in Settings->Plug-ins choose "Individually" plugin setting and check all of the LinVst plugins.
For Bitwig 2.4.3, In Settings->Plug-ins choose Independent plug-in host process for "Each plug-in" setting and check all of the LinVst plugins.

**Renoise**

Choose the sandbox option for plugins.

Sometimes a synth vst might not declare itself as a synth and Renoise might not enable it.

A workaround is to install sqlitebrowser

sudo add-apt-repository ppa:linuxgndu/sqlitebrowser-testing

sudo apt-get update && sudo apt-get install sqlitebrowser

Add the synth vst's path to VST_PATH and start Renoise to scan it.

Then exit renoise and edit the database file /home/user/.renoise/V3.1.0/ CachedVSTs_x64.db (enable hidden folders with right click in the file browser).

Go to the "Browse Data" tab in SQLite browser and choose the CachedPlugins table and then locate the entry for the synth vst and enable the "IsSynth" flag from "0" (false) to "1" (true) and save.

-----------

**d2d1**

If a plugin needs a dll override it might be found by running TestVst from the terminal and looking for any unimplemented function in xxxx.dll errors in the output (the xxxx.dll is the dll that needs to be replaced with a dll override).

d2d1 based plugins

d2d1.dll can cause errors because Wine's current d2d1 support is not complete and using a d2d1.dll override might produce a black (blank) display.

Some plugins might need d2d1 to be disabled in the winecfg Libraries tab (add a d2d1 entry and then edit it and select disable), but some plugins won't run if d2d1 is disabled.

A scan of the plugin dll file can be done to find out if the plugin depends on d2d1 

"strings vstname.dll | grep -i d2d1"

--------
