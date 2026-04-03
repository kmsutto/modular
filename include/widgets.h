#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

void widgetDrawClock(int x, int y, const String &t, int size = 3);
void widgetDrawDate(int x, int y, const String &d, int size = 1);
void widgetDrawWeather(int x, int y, int size = 1);
void widgetSetWeather(const String &condition, const String &temp);
void widgetUpdateWeatherFade();
