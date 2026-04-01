<div align="center">
<img width="801" height="277" src="media/readme-picture.png" />
</div>

## information
**modular** is an open-source firmware for ESP32 which turns your device into a mini desktop clock with the weather and much more. 
also, you can always send a pull request, we are open to this and will only be glad

to connect all the modules (screen, joystick) correctly, read the [wire-guide.md](media/wire.md) instructions to connect everything correctly

> [!warning]
> the project is in the beta stage, so I do not guarantee stability.
---

## widgets and their work
"widgets" in this firmware are elements that you completely customize for yourself, and you can always create your own widgets with an api, steady hands and a couple of minutes of time.

> clock //
> a simple widget that shows the time set through the initial setting after flashing 

> date // 
> shows a real date and a real number (lmao really real), without any settings 

> weather //
> it works with the OpenWeather api key specification and also with the city specification in the same initial setting 
---


## software requirements

- [vscode](https://code.visualstudio.com/)
- [platformio](https://platformio.org/install/ide?install=vscode)

---

## build & flash

```bash
# build firmware
pio run

# upload to ESP32
pio run --target upload
