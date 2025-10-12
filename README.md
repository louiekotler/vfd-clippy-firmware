# vfd-clippy-firmware
Arduino firmware for VFD Clippy

## Setup
1. Copy `hardware/` to your Arduino sketchbook directory to add the custom board definition
    * i.e. `cp ./hardware ~/Documents/Arduino/`
    * [More details on where the sketchbook folder is located](https://support.arduino.cc/hc/en-us/articles/4412950938514-Open-the-Sketchbook-folder)
1. Restart the Arduino IDE to capture the new board definition
1. Select `Tools -> Board -> VFD Clippy (in Sketchbook) -> Arduino Zero (Native USB Port)`
1. If needed, select `Tools -> Port -> {your_port}`
1. Click `Upload`