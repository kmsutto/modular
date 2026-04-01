#pragma once
#include <Arduino.h>
#include <Preferences.h>

// -- ap & mdns settings
#define AP_SSID     "modular"
#define MDNS_NAME   "modular"

// -- firmware settings
struct AppSettings {
    String wifiSSID;
    String wifiPassword;
    String owmApiKey;
    String city;
    int    widgetClockX;
    int    widgetClockY;
    int    widgetDateX;
    int    widgetDateY;
    int    widgetWeatherX;
    int    widgetWeatherY;
};

// -- settings manager
class SettingsManager {
public:
    AppSettings data;

    void load() {
        Preferences p;
        p.begin("modular", true);
        data.wifiSSID       = p.getString("ssid",    "");
        data.wifiPassword   = p.getString("pass",    "");
        data.owmApiKey      = p.getString("owmkey",  "");
        data.city           = p.getString("city",    "Kyiv");
        data.widgetClockX   = p.getInt("cx",  14);
        data.widgetClockY   = p.getInt("cy",  10);
        data.widgetDateX    = p.getInt("dx",  20);
        data.widgetDateY    = p.getInt("dy",  38);
        data.widgetWeatherX = p.getInt("wx",  10);
        data.widgetWeatherY = p.getInt("wy",  52);
        p.end();
    }

    void save() {
        Preferences p;
        p.begin("modular", false);
        p.putString("ssid",   data.wifiSSID);
        p.putString("pass",   data.wifiPassword);
        p.putString("owmkey", data.owmApiKey);
        p.putString("city",   data.city);
        p.putInt("cx", data.widgetClockX);
        p.putInt("cy", data.widgetClockY);
        p.putInt("dx", data.widgetDateX);
        p.putInt("dy", data.widgetDateY);
        p.putInt("wx", data.widgetWeatherX);
        p.putInt("wy", data.widgetWeatherY);
        p.end();
    }

    bool isConfigured() {
        return data.wifiSSID.length() > 0;
    }
};

extern SettingsManager settings;
