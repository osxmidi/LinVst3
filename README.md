# LinVst3

Vst3 wrapper (beta).

For 64 bit vst3's only.

Not all vst3 features are supported.

Unlike Vst2, saved projects using wrapped Vst3 plugins are not compatible/transferable between Linux daws and their Windows versions and vice versa.

LinVst3 binaries are on the releases page (under Assets) https://github.com/osxmidi/LinVst3/releases

See https://github.com/osxmidi/LinVst3/tree/master/Tested-VST3-Plugins for some common Vst3 tested plugin details.

The same usage applies as per LinVst except that it's linvst3.so instead of linvst.so and the vst dll filename extensions are .vst3 instead of .dll https://github.com/osxmidi/LinVst/wiki https://github.com/osxmidi/LinVst/blob/master/README.md https://github.com/osxmidi/LinVst/tree/master/Detailed-Guide

So for example, linvst3.so would be renamed to Delay.so for Delay.vst3 (see convert folder for batch name conversion utilities)

The vst3 dlls are most likely going to be in ~/.wine/drive_c/Program Files/Common Files/VST3

LinVst3 will try to produce multiple loader part files for vst3's that contain multiple plugins. 
The multiple loader part files should be picked up on the daw's next plugin scan and then the multiple plugins should be available for use in the daw.

If window resizing does not work, then after a resize the UI needs to be closed and then reopened for the new window size to take effect.

Some vst3 plugins might not work due to Wines current capabilities or for some other reason.

Use TestVst3 for testing how a vst3 plugin might run under Wine.

Some vst3 plugins rely on the d2d1 dll which is not totally implemented in current Wine.

If a plugin has trouble with it's display then disabling d2d1 in the winecfg Libraries tab can be tried.

-----------

Optional Symlinks

A symlink can be used to access vst3 plugin folders from another more convenient folder.

Hidden folders such as /home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3 can be accessed by the Daw by creating a symlink to them using a more convenient folder such as /home/your-user-name/vst3 for instance.

For example

ln -s "/home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3" /home/your-user-name/vst3/vst3plugins.so

creates a symbolic link named vst3plugins.so in the /home/your-user-name/vst3 folder that points to the /home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3 folder containing the vst3 plugins.

The /home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3 vst3 plugin folder needs to have had the vst3 plugins previously setup by using linvst3convert.

Then the Daw needs to have the /home/your-user-name/vst3 folder included in it's search path.

When the Daw scans the /home/your-user-name/vst3 folder it should also automatically scan the /home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3 folder that contains the vst3 plugins (that have been previously setup by using linvst3convert).

-------

**Hyperthreading**

For Reaper, in Options/Preferences/Buffering uncheck Auto-detect the number of needed audio processing threads and set 
Audio reading/processing threads to the amount of physical cores of the cpu (not virtual cores such as hyperthreading cores).

This can help with stutters and rough audio response.

Other Daws might have similar settings.

**Waveform**

For Waveform,(maybe) disable sandbox option for plugins.

**Bitwig**

For Bitwig, in Settings->Plug-ins choose "Individually" plugin setting and check all of the LinVst plugins.
For Bitwig 2.4.3, In Settings->Plug-ins choose Independent plug-in host process for "Each plug-in" setting and check all of the LinVst3 plugins.

**Renoise**

Choose the sandbox option for plugins.

-------

**To Make**

LinVst3 binaries are on the releases page (under Assets) https://github.com/osxmidi/LinVst3/releases

See https://github.com/osxmidi/LinVst/tree/master/Make-Guide for setup info and make options

The vst3 sdk needs to be patched and the default LinVst3 script patch file (lin-patchwin) is for this version of the Steinberg vst sdk https://download.steinberg.net/sdk_downloads/vst-sdk_3.7.1_build-50_2020-11-17.zip

Some libraries also need to be pre installed, 

For Ubuntu/Debian:

sudo apt-get install libx11-dev

sudo apt-get install wine-stable-dev or sudo apt-get install wine-staging-dev

sudo apt-get install cmake

sudo apt-get install libfreetype6-dev libxcb-util0-dev libxcb-cursor-dev libxcb-keysyms1-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libgtkmm-3.0-dev libsqlite3-dev

For Manjaro/Arch: 

sudo pacman -Sy wine (or wine-staging) libx11 gcc-multilib

sudo pacman -Sy cmake freetype2 sqlite libxcb xcb-util gtkmm3 xcb-util-cursor libx11 pkgconfig xcb-util-keysyms

For Fedora:

sudo yum -y install wine-devel libX11-devel

sudo yum -y install sqlite sqlite-devel cmake freetype-devel xcb-util-devel gcc g++ xcb-util-cursor xcb-util-cursor-devel xcb-util-keysyms xcb-util-keysyms-devel libxkbcommon libxkbcommon-devel libxkbcommon-x11 libxkbcommon-x11-devel gtk+ gtk+-devel gtk3 gtk3-devel gtkmm3.0 gtkmm3.0-devel

optional libraries xcb-util libX11-xcb 

------

(Optional libraries, Maybe needed for some systems),

libx11-xcb-dev
libxcb-util-dev
libxcb-cursor-dev
libxcb-xkb-dev
libxkbcommon-dev
libxkbcommon-x11-dev
libfontconfig1-dev
libcairo2-dev
libgtkmm-3.0-dev
libsqlite3-dev
libxcb-keysyms1-dev

-------

This LinVst3 source folder needs to be placed within the VST3 SDK main folder (the VST3_SDK folder or the VST3 folder that contains the base, public.sdk, pluginterfaces etc folders) ie the LinVst3 source folder needs to be placed alongside the base, public.sdk, pluginterfaces etc folders of the VST3 SDK.

Then change into the LinVst3 source folder and run make and then sudo make install

Then use the batch name conversion utilities (in the convert/binaries folder) to name convert linvst3.so to the vst3 plugin names ie first select linvst3.so and then select the ~/.wine/drive_c/Program Files/Common Files/VST3 folder https://github.com/osxmidi/LinVst/wiki

To make using the vst2sdk, remove the -DVESTIGE entries from the Makefile and place the vst2sdk pluginterfaces folder inside the main LinVst3 source folder.

----------

````//-----------------------------------------------------------------------------
// LICENSE
// (c) 2018, Steinberg Media Technologies GmbH, All Rights Reserved
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
