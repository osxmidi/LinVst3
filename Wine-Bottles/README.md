Install approx guide


create bottle named vstbottle (or whatever)


(To check) ls ~/.var/app/com.usebottles.bottles/data/bottles/bottles/vstbottle (the ~/ at the start of the path means your /home/username/)


run executable to install a vst3 plugin

ie the vst3 plugin was installed into "Program Files"/"Common Files"/VST3 (might be installed to other locations in "Program Files" depending on the plugin ie could be installed into the Steinberg foder instead of VST3 or whatever)

Use File Manager to see where the vst3 was installed in the bottle.

(To check) ~/.var/app/com.usebottles.bottles/data/bottles/bottles/vstbottle/drive_c/"Program Files"/"Common Files"/VST3


use linvst3convert and choose the destination to be where the vst3 was installed 

~/var/app/com.usebottles.bottles/data/bottles/bottles/vstbottle/drive_c/"Program Files"/"Common Files"/VST3


add the Daw VST search path to include ~/var/app/com.usebottles.bottles/data/bottles/bottles/vstbottle/drive_c/"Program Files"/"Common Files"/VST3


The WINELOADER variable needs to be set as it points to the location where wine is (wine is usually in /usr/bin but with bottles it's not).

USE (depending where wine is in a bottle)

export WINELOADER=~/.var/app/com.usebottles.bottles/data/bottles/runners/soda-9.0-1/bin/wine (in a terminal)

to set the WINELOADER

Run Daw from the terminal (or configure WINELOADER to be set on boot)

LinVst3 automatically sets the WINEPREFIX to the wine location of the vst3 when the Daw loads the vst3.

check if the WINELOADER path is set

env | grep WINELOADER

check wine version

$WINELOADER --version
