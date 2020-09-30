# TestVst3

TestVst3 can roughly test how a vst3 plugin might run under Wine.

It tries to open the vst3 and display the vst3 window for roughly 8 seconds and then closes.

Usage is ./testvst3.exe "vst3file.vst3"

paths and vst3 filenames that contain spaces need to be enclosed in quotes.

testvst3.exe is for 64 bit vst3's only

If TestVst3 is run from a terminal and the output is looked at, then sometimes there can be an unimplemented function error in some dll and that dll can then be overriden.

Some vst3's might not work due to Wines current capabilities or for some other reason.

Some vst3 plugins rely on the d2d1 dll which is not totally implemented in current Wine.

If a plugin has trouble with it's display then disabling d2d1 in the winecfg Libraries tab can be tried.

The Sforzando VST3 runs in a better way with d2d1 disabled for instance.

-----

Batch Testing

For testing multiple vst3 files at once, place testvst3.exe and testvst3.exe.so and testvst3-batch into the vst3 folder containing the vst3 files.

(remove the testvst3 files from any daw search paths after testing).

Using the terminal, cd into the vst3 folder and enter

chmod +x testvst3-batch

then enter

./testvst3-batch

After that, testvst3.exe will attempt to run the plugins one after another, any plugin dialogs that popup should be dismissed as soon as possible.

If a Wine plugin problem is encountered, then that plugin can be identified by the terminal output from testvst3.exe.

------

To make

This TestVst3 source folder needs to be placed within the VST3 SDK main folder (the VST3_SDK folder or the VST3 folder that contains the base, public.sdk, pluginterfaces etc folders) ie the TestVst3 source folder needs to be placed alongside the base, public.sdk, pluginterfaces etc folders of the VST3 SDK.

Then change into the TestVst3 folder and run make.

------

Cmake needs to be preinstalled.

sudo apt-get install cmake

Libraries that need to be pre installed, 

sudo apt-get install libfreetype6-dev libxcb-util0-dev libxcb-cursor-dev libxcb-keysyms1-dev libxcb-xkb-dev libxkbcommon-dev libxkbcommon-x11-dev libgtkmm-3.0-dev libsqlite3-dev

Wine libwine development files.

------

For Ubuntu/Debian, sudo apt-get install libwine-development-dev (For Debian, Wine might need to be reinstalled after installing libwine-development-dev)

wine-devel packages for other distros (sudo apt-get install wine-devel).

libX11 development (sudo apt-get install libx11-dev)

------

For Fedora 

sudo yum -y install wine-devel wine-devel.i686 libX11-devel libX11-devel.i686
sudo yum -y install libstdc++.i686 libX11.i686

------

For Manjaro/Arch

sudo pacman -Sy wine-staging libx11 gcc-multilib

sudo pacman -Sy cmake freetype2 sqlite libxcb xcb-util gtkmm3 xcb-util-cursor

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
