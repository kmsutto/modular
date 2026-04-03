#pragma once
#include <Arduino.h>

#define SCREEN_WIDTH        128
#define SCREEN_HEIGHT        64
#define OLED_RESET           -1
#define OLED_ADDRESS       0x3C

#define NTP_SERVER        "pool.ntp.org"
#define NTP_OFFSET         10800
#define NTP_INTERVAL       60000

#define WEATHER_UPDATE_MS  600000
#define CLOCK_UPDATE_MS      1000
#define WEATHER_FADE_STEPS     16
#define WEATHER_FADE_TICK_MS   40

#define WEATHER_ICON_W        10
#define WIFI_RECONNECT_MS  30000

#define AP_SSID    "modular"
#define MDNS_NAME  "modular"
