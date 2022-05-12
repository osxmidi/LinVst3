## Tested Vst3's

LinVst tested with Waveform, Ardour, Reaper, Renoise, Bitwig Studio (For Bitwig 2.5 and later, In Settings->Plug-ins choose "Individually" plugin setting and check all of the LinVst plugins.
For Bitwig 2.4.3, In Settings->Plug-ins choose Independent plug-in host process for "Each plug-in" setting and check all of the LinVst plugins).

If a plugin needs a dll override it can be found by running TestVst from the terminal and looking for any unimplemented function in xxxx.dll errors in the output (the xxxx.dll is the dll that needs to be replaced with a dll override).

d2d1 based plugins

d2d1.dll can cause errors because Wine's current d2d1 support is not complete and using a d2d1.dll override might produce a black (blank) display.

Some plugins might need d2d1 to be disabled in the winecfg Libraries tab (add a d2d1 entry and then edit it and select disable).

A scan of the plugin dll file can be done to find out if the plugin depends on d2d1 

"strings vstname.dll | grep -i d2d1"

**Audio/Midi clips**
LinVst has a Drag-and-Drop version (see the dragwin folder for the Makefile).
The Audacity Windows version can be used to drag audio/midi clips from the vst window to Audacity (after maybe previewing/arranging them in the vst such as MT-PowerDrumKit and EZdrummer) and then there is the option of editing them in Audacity before saving and dragging them to the Linux Daw.
Drag the clip from the plugin window to Audacity, then select the track in Audacity and export it, then drag it to the Daw.

----------------------

**Kontakt** 

**EZDrummer** 

**Valhalla** 

**Melda** 
Installer might need corefonts (winetricks corefonts)
Melda MXXX Multi Effects** (turn GPU acceleration off)

**Blue Cat** 

**Voxengo** 

**Fabfilter** 

**Mercuriall Spark Amp Sim**



