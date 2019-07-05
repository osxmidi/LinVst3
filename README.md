# LinVst3

Vst3 wrapper.

For 64 bit vst3's only.

Not all vst3 features are supported.

Same usage as LinVst except that it's linvst3.so instead of linvst.so

The vst3 dlls are most likely going to be in ~/.wine/drive_c/Program Files/Common Files/VST3

-------

To Make

Libraries that need to be pre installed

libfreetype6-dev
libxcb-util0-dev
libxcb-cursor-dev
libxcb-keysyms1-dev
libxcb-xkb-dev
libxkbcommon-dev
libxkbcommon-x11-dev
libgtkmm-3.0-dev
libsqlite3-dev

Wine libwine development files.

For Ubuntu/Debian, sudo apt-get install libwine-development-dev (for Ubuntu 14.04 it's sudo apt-get install wine1.8 and sudo apt-get install wine1.8-dev)

wine-devel packages for other distros (sudo apt-get install wine-devel).

libX11 development needed for embedded version (sudo apt-get install libx11-dev)

For Fedora 
sudo yum -y install wine-devel wine-devel.i686 libX11-devel libX11-devel.i686
sudo yum -y install libstdc++.i686 libX11.i686

This LinVst3 folder needs to be placed within the VST3 SDK main folder (ie the VST3 folder that contains the base public.sdk pluginterfaces folders etc)

Then change into the LinVst3 folder and run make

Currently builds ok with the vstsdk3613_08_04_2019_build_81
