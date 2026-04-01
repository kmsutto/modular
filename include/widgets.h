#pragma once
#include <Arduino.h>
#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

// -- widgets preferences
void widgetDrawClock(int x, int y, const String &t);
void widgetDrawDate(int x, int y, const String &d);
void widgetDrawWeather(int x, int y);
void widgetSetWeather(const String &condition, const String &temp);
void widgetUpdateWeatherFade();
void widgetDrawEditorCursor(int x, int y, int w, int h);