# LinVst3

Vst3 wrapper (beta).

For 64 bit vst3's only.

Not all vst3 features are supported.

Same usage applies as per LinVst except that it's linvst3.so instead of linvst.so and the vst dll filename extensions are .vst3 instead of .dll https://github.com/osxmidi/LinVst/wiki https://github.com/osxmidi/LinVst/blob/master/README.md https://github.com/osxmidi/LinVst/tree/master/Detailed-Guide

So for example, linvst3.so would be renamed to Delay.so for Delay.vst3 (see convert folder for batch name conversion utilities)

The vst3 dlls are most likely going to be in ~/.wine/drive_c/Program Files/Common Files/VST3

LinVst3 will try to produce multiple loader part files for vst3's that contain multiple plugins. 
The multiple loader part files should be picked up on the daw's next plugin scan and then the multiple plugins should be available for use in the daw.

Some vst3's might not work due to Wines current capabilities or for some other reason.

-------

To Make

sudo apt-get cmake

Libraries that need to be pre installed, 

sudo apt-get install libfreetype6-dev libxcb-util0-dev libxcb-cursor-dev libxcb-keysyms1-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libgtkmm-3.0-dev libsqlite3-dev

Wine libwine development files.

For Ubuntu/Debian, sudo apt-get install libwine-development-dev (For Debian, Wine might need to be reinstalled after installing libwine-development-dev)

wine-devel packages for other distros (sudo apt-get install wine-devel).

libX11 development (sudo apt-get install libx11-dev)

For Fedora 
sudo yum -y install wine-devel wine-devel.i686 libX11-devel libX11-devel.i686
sudo yum -y install libstdc++.i686 libX11.i686

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

Then change into the LinVst3 folder and run make and then sudo make install

Then use the batch name conversion utilities (in the convert/binaries folder) to name convert linvst3.so to the vst3 plugin names ie first select linvst3.so and then select the ~/.wine/drive_c/Program Files/Common Files/VST3 folder https://github.com/osxmidi/LinVst/wiki

Currently builds ok with the vstsdk3613_08_04_2019_build_81

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
