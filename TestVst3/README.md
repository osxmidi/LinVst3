# TestVst3

TestVst3 can roughly test how a vst3 dll plugin might run under Wine.

It tries to open the vst3 dll and display the vst window for roughly 8 seconds and then closes.

Usage is ./testvst3.exe "path to vst3file.vst3"

paths and vst3 filenames that contain spaces need to be enclosed in quotes.

for example (cd into the testvst3 folder using the terminal)

./testvst3.exe "/home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3/delay.vst3"

Use testvst3.exe from a folder that is not in a daw search path.

If testvst3.exe.so is in any daw search path then it can cause problems if the daw tries to load it.

-----

Batch Testing

For testing multiple vst3 dll files at once, unzip the testvst3 folder, then (using the terminal) cd into the unzipped testvst3 folder.

then enter

chmod +x testvst3-batch

Usage is ./testvst3-batch "path to the vst3 folder containing the vst3 dll files"

paths that contain spaces need to be enclosed in quotes.

pathnames must end with a /

for example

./testvst-batch3 "/home/your-user-name/.wine/drive_c/Program Files/Common Files/VST3/"

After that, testvst3.exe will attempt to run the plugins one after another, any plugin dialogs that popup should be dismissed as soon as possible.

If a Wine plugin problem is encountered, then that plugin can be identified by the terminal output from testvst3.exe.

Use testvst3.exe from a folder that is not in a daw search path.

Use testvst3.exe from a folder that is not in a daw search path.

If testvst3.exe.so is in any daw search path then it can cause problems if the daw tries to load it.

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
