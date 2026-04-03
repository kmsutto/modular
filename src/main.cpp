#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ESPmDNS.h> 
#include "config.h"
#include "settings.h"
#include "widgets.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SettingsManager  settings;
WebServer        server(80);
WiFiUDP          ntpUDP;
NTPClient        timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, NTP_INTERVAL);

static unsigned long lastWeatherMs   = 0;
static unsigned long lastClockMs     = 0;
static unsigned long lastFadeMs      = 0;
static unsigned long lastReconnectMs = 0;

static String cachedTime;
static String cachedDate;
static String cachedTemp = "0";

static const char *DAYS[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
static const char *MONTHS[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                "Jul","Aug","Sep","Oct","Nov","Dec"};

static String readFile(const char *path) {
    File f = SPIFFS.open(path, "r");
    if (!f) return "";
    String s = f.readString();
    f.close();
    return s;
}

static void buildTimeDate() {
    timeClient.update();
    int h = timeClient.getHours();
    int m = timeClient.getMinutes();
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", h, m);
    cachedTime = String(buf);

    time_t epoch = timeClient.getEpochTime();
    struct tm *ti = localtime(&epoch);
    char dbuf[24];
    snprintf(dbuf, sizeof(dbuf), "%s, %d %s",
             DAYS[ti->tm_wday], ti->tm_mday, MONTHS[ti->tm_mon]);
    cachedDate = String(dbuf);
}

static void fetchWeather() {
    if (settings.data.owmApiKey.isEmpty()) return;
    if (WiFi.status() != WL_CONNECTED) return;

    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" +
                 settings.data.city +
                 "&appid=" + settings.data.owmApiKey +
                 "&units=metric";
    http.begin(url);
    if (http.GET() == 200) {
        JsonDocument doc;
        deserializeJson(doc, http.getString());
        int temp = (int)round((float)doc["main"]["temp"]);
        cachedTemp = String(temp);
        widgetSetWeather(doc["weather"][0]["main"].as<String>(), cachedTemp);
    }
    http.end();
}

static void drawOnboarding() {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("[ modular ]");
    display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
    display.setCursor(0, 14);  display.print("Setup your modular device!");
    display.setCursor(0, 26);  display.print("Go to: modular.local");
    display.display();
}

static void drawMain() {
    display.clearDisplay();

    if (settings.data.widgetClockVisible)
        widgetDrawClock(settings.data.widgetClockX, settings.data.widgetClockY,
                        cachedTime, settings.data.widgetClockSize);

    if (settings.data.widgetDateVisible)
        widgetDrawDate(settings.data.widgetDateX, settings.data.widgetDateY,
                       cachedDate, settings.data.widgetDateSize);

    if (settings.data.widgetWeatherVisible)
        widgetDrawWeather(settings.data.widgetWeatherX, settings.data.widgetWeatherY,
                          settings.data.widgetWeatherSize);

    display.display();
}

void setupWebServer() {
    server.enableCORS(true);

    server.on("/", HTTP_GET, []() {
        String html = readFile("/index.html");
        server.send(200, "text/html", html.isEmpty() ? "Error: SPIFFS Error" : html);
    });

    server.on("/config", HTTP_GET, []() {
        JsonDocument doc;
        doc["configured"] = settings.isConfigured();
        doc["ssid"]       = settings.data.wifiSSID;
        doc["city"]       = settings.data.city;
        doc["owmkey"]     = settings.data.owmApiKey;
        doc["connected"]  = (WiFi.status() == WL_CONNECTED);
        doc["brightness"] = settings.data.brightness;

        doc["clockX"]   = settings.data.widgetClockX;
        doc["clockY"]   = settings.data.widgetClockY;
        doc["clockSize"]= settings.data.widgetClockSize;
        doc["clockVis"] = settings.data.widgetClockVisible;
        doc["dateX"]    = settings.data.widgetDateX;
        doc["dateY"]    = settings.data.widgetDateY;
        doc["dateSize"] = settings.data.widgetDateSize;
        doc["dateVis"]  = settings.data.widgetDateVisible;
        doc["weatherX"] = settings.data.widgetWeatherX;
        doc["weatherY"] = settings.data.widgetWeatherY;
        doc["weatherSize"] = settings.data.widgetWeatherSize;
        doc["weatherVis"]  = settings.data.widgetWeatherVisible;

        doc["cachedTime"] = cachedTime;
        doc["cachedDate"] = cachedDate;
        doc["cachedTemp"] = cachedTemp;

        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    server.on("/save", HTTP_POST, []() {
        if (server.hasArg("ssid"))   settings.data.wifiSSID     = server.arg("ssid");
        if (server.hasArg("pass"))   settings.data.wifiPassword = server.arg("pass");
        if (server.hasArg("owmkey")) settings.data.owmApiKey    = server.arg("owmkey");
        if (server.hasArg("city"))   settings.data.city         = server.arg("city");

        settings.save();
        server.send(200, "application/json", "{\"ok\":true}");

        if (server.hasArg("restart") && server.arg("restart") == "1") {
            delay(500);
            ESP.restart();
        }
    });

    server.on("/layout", HTTP_POST, []() {
        if (server.hasArg("clockX"))    settings.data.widgetClockX      = server.arg("clockX").toInt();
        if (server.hasArg("clockY"))    settings.data.widgetClockY      = server.arg("clockY").toInt();
        if (server.hasArg("clockSize")) settings.data.widgetClockSize   = server.arg("clockSize").toInt();
        if (server.hasArg("clockVis"))  settings.data.widgetClockVisible = (server.arg("clockVis") == "1");
        if (server.hasArg("dateX"))     settings.data.widgetDateX       = server.arg("dateX").toInt();
        if (server.hasArg("dateY"))     settings.data.widgetDateY       = server.arg("dateY").toInt();
        if (server.hasArg("dateSize"))  settings.data.widgetDateSize    = server.arg("dateSize").toInt();
        if (server.hasArg("dateVis"))   settings.data.widgetDateVisible = (server.arg("dateVis") == "1");
        if (server.hasArg("weatherX"))  settings.data.widgetWeatherX    = server.arg("weatherX").toInt();
        if (server.hasArg("weatherY"))  settings.data.widgetWeatherY    = server.arg("weatherY").toInt();
        if (server.hasArg("weatherSize")) settings.data.widgetWeatherSize = server.arg("weatherSize").toInt();
        if (server.hasArg("weatherVis"))  settings.data.widgetWeatherVisible = (server.arg("weatherVis") == "1");

        settings.save();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/brightness", HTTP_POST, []() {
        if (server.hasArg("value")) {
            settings.data.brightness = server.arg("value").toInt();
            display.ssd1306_command(SSD1306_SETCONTRAST);
            display.ssd1306_command(settings.data.brightness);
            settings.save();
        }
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.begin();
}

void setup() {
    Serial.begin(115200);
    Wire.begin();

    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.clearDisplay();
    display.display();
    display.setRotation(2);

    if (!SPIFFS.begin(true)) {
        Serial.println("[!] spiffs error");
    }

    settings.load();

    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(settings.data.brightness);

    WiFi.mode(WIFI_STA);
    
    if (settings.isConfigured()) {
        WiFi.begin(settings.data.wifiSSID.c_str(), settings.data.wifiPassword.c_str());
        
        unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 10000) {
            delay(100);
        }
    }

    if (WiFi.status() != WL_CONNECTED) {
        WiFi.mode(WIFI_AP_STA);
        WiFi.softAP(AP_SSID);
        Serial.println("Wi-Fi failed, AP started");
    } else {
        if (MDNS.begin("modular")) {
            Serial.println("mDNS: http://modular.local");
        }
        timeClient.begin();
        buildTimeDate();
        fetchWeather();
    }

    setupWebServer();

    lastWeatherMs = lastClockMs = lastFadeMs = lastReconnectMs = millis();
}

void loop() {
    server.handleClient();

    if (!settings.isConfigured()) {
        drawOnboarding();
        return;
    }

    unsigned long now = millis();

    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastReconnectMs > 30000) {
            WiFi.reconnect();
            lastReconnectMs = now;
        }
    }

    if (now - lastFadeMs    > 50)     { widgetUpdateWeatherFade(); lastFadeMs = now; }
    if (now - lastClockMs   > 1000)   { buildTimeDate();           lastClockMs = now; }
    if (now - lastWeatherMs > 600000) { fetchWeather();            lastWeatherMs = now; }

    drawMain();
}