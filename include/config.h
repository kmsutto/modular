#pragma once
#include <Arduino.h>

// -- ssd1306
#define SCREEN_WIDTH     128
#define SCREEN_HEIGHT     64
#define OLED_RESET        -1
#define OLED_ADDRESS    0x3C

// -- analOg jouytick
#define JOY_X_PIN         34
#define JOY_Y_PIN         35
#define JOY_BTN_PIN       32

#define JOY_CENTER       2048
#define JOY_DEAD_ZONE     300
#define JOY_LONG_PRESS_MS 800

// -- features
#define NTP_SERVER       "pool.ntp.org"
#define NTP_OFFSET       10800
#define NTP_INTERVAL     60000

#define WEATHER_UPDATE_MS 600000
#define CLOCK_UPDATE_MS    1000
#define WEATHER_FADE_STEPS   16
