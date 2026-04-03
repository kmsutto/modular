#pragma once
#include <Arduino.h>
#include <Preferences.h>

#define AP_SSID     "modular"
#define MDNS_NAME   "modular"

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

    int    widgetClockSize;
    int    widgetDateSize;
    int    widgetWeatherSize;

    bool   widgetClockVisible;
    bool   widgetDateVisible;
    bool   widgetWeatherVisible;

    int    brightness;
};

class SettingsManager {
public:
    AppSettings data;

    void load() {
        Preferences p;
        p.begin("modular", true);
        data.wifiSSID       = p.getString("ssid",    "");
        data.wifiPassword   = p.getString("pass",    "");
        data.owmApiKey      = p.getString("owmkey",  "");
        data.city           = p.getString("city",    "new-york");

        data.widgetClockX   = p.getInt("cx",  14);
        data.widgetClockY   = p.getInt("cy",  10);
        data.widgetDateX    = p.getInt("dx",  20);
        data.widgetDateY    = p.getInt("dy",  38);
        data.widgetWeatherX = p.getInt("wx",  10);
        data.widgetWeatherY = p.getInt("wy",  52);

        data.widgetClockSize    = p.getInt("csz", 3);
        data.widgetDateSize     = p.getInt("dsz", 1);
        data.widgetWeatherSize  = p.getInt("wsz", 1);

        data.widgetClockVisible   = p.getBool("cvis", true);
        data.widgetDateVisible    = p.getBool("dvis", true);
        data.widgetWeatherVisible = p.getBool("wvis", true);

        data.brightness = p.getInt("bright", 200);
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

        p.putInt("csz", data.widgetClockSize);
        p.putInt("dsz", data.widgetDateSize);
        p.putInt("wsz", data.widgetWeatherSize);

        p.putBool("cvis", data.widgetClockVisible);
        p.putBool("dvis", data.widgetDateVisible);
        p.putBool("wvis", data.widgetWeatherVisible);

        p.putInt("bright", data.brightness);
        p.end();
    }

    bool isConfigured() {
        return data.wifiSSID.length() > 0;
    }
};

extern SettingsManager settings;
