## wiring
this guide explains how to connect an **esp32**, a **0.96" oled (ssd1306)**, and an **analog joystick** using a dual-breadboard setup for maximum stability.

## components Required
* **esp32 devkit v1** (30-pin version)
* **oled display** (0.96 inch, ssd1306, i2c interface)
* **2x mini breadboards**
* **jumper wires** (male-to-male and male-to-female)

<img width="900" height="700" alt="1" src="https://github.com/user-attachments/assets/df36e135-77a4-4aa3-9005-c39882232366" />

## step 1: base setup 
standard esp32 boards are too wide for a single mini breadboard, leaving no room for jumper wires. 

1. take **two mini breadboards** and snap them together side-by-side.
2. insert the **esp32** across the center bridge of both boards. this ensures that every pin on the esp32 has an accessible row on the breadboard.

<img width="900" height="700" alt="2" src="https://github.com/user-attachments/assets/44a9d6a0-654d-4e08-887e-40c425d54089" />

## step 2: wiring the oled display
the oled uses the **i2c protocol**, which requires only 4 wires. connect them as follows:

| ssd1306 pin | esp32 pin | function |
| :--- | :--- | :--- |
| **gnd** | **gnd** | ground |
| **vcc** | **3v3** | power |
| **scl** | **d22** | serial clock |
| **sda** | **d21** | Serial data |

<img width="900" height="700" alt="3" src="https://github.com/user-attachments/assets/1cc69fa0-59c2-4147-b2fc-dbfb056ac9ec" />

## step 3: you are beautiful!
now you can put it on the shelf, connected type-c, on the table, anywhere. all you have to do is compile the firmware and flash it! 

---
made by kmsutto <3
