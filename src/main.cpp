#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <NTPClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "config.h"
#include "settings.h"
#include "widgets.h"

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
SettingsManager  settings;
WebServer        server(80);
WiFiUDP          ntpUDP;
NTPClient        timeClient(ntpUDP, NTP_SERVER, NTP_OFFSET, NTP_INTERVAL);

unsigned long lastWeatherMs = 0;
unsigned long lastClockMs   = 0;
unsigned long lastFadeMs    = 0;

String cachedTime;
String cachedDate;
String cachedTemp = "0";

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
    const char *days[]   = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
    const char *months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                             "Jul","Aug","Sep","Oct","Nov","Dec"};
    char dbuf[24];
    snprintf(dbuf, sizeof(dbuf), "%s, %d %s",
             days[ti->tm_wday], ti->tm_mday, months[ti->tm_mon]);
    cachedDate = String(dbuf);
}

static void fetchWeather() {
    if (settings.data.owmApiKey.isEmpty()) return;
    HTTPClient http;
    String url = "http://api.openweathermap.org/data/2.5/weather?q=" + settings.data.city +
                 "&appid=" + settings.data.owmApiKey + "&units=metric";
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

    display.setTextSize(1);
    display.setCursor(0, 14);
    display.print("wi-fi: modular");

    display.setCursor(0, 26);
    display.print("open browser:");

    display.setCursor(0, 36);
    display.setTextColor(SSD1306_WHITE);
    display.print("modular.local");

    static bool blink = false;
    static unsigned long lastBlink = 0;
    if (millis() - lastBlink > 600) { blink = !blink; lastBlink = millis(); }
    if (blink) display.fillCircle(SCREEN_WIDTH - 4, SCREEN_HEIGHT - 4, 2, SSD1306_WHITE);

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

static void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        String html = readFile("/index.html");
        server.send(200, "text/html", html.isEmpty() ? "Error loading page" : html);
    });

    server.on("/config", HTTP_GET, []() {
        JsonDocument doc;
        bool configured = settings.isConfigured();
        doc["configured"] = configured;
        doc["ssid"]       = settings.data.wifiSSID;
        doc["city"]       = settings.data.city;
        doc["owmkey"]     = settings.data.owmApiKey;
        doc["connected"]  = (WiFi.status() == WL_CONNECTED);
        doc["ip"]         = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";

        doc["clockX"]    = settings.data.widgetClockX;
        doc["clockY"]    = settings.data.widgetClockY;
        doc["clockSize"] = settings.data.widgetClockSize;
        doc["clockVis"]  = settings.data.widgetClockVisible ? 1 : 0;

        doc["dateX"]     = settings.data.widgetDateX;
        doc["dateY"]     = settings.data.widgetDateY;
        doc["dateSize"]  = settings.data.widgetDateSize;
        doc["dateVis"]   = settings.data.widgetDateVisible ? 1 : 0;

        doc["weatherX"]    = settings.data.widgetWeatherX;
        doc["weatherY"]    = settings.data.widgetWeatherY;
        doc["weatherSize"] = settings.data.widgetWeatherSize;
        doc["weatherVis"]  = settings.data.widgetWeatherVisible ? 1 : 0;

        doc["cachedTime"] = cachedTime;
        doc["cachedDate"] = cachedDate;
        doc["cachedTemp"] = cachedTemp;
        doc["brightness"] = settings.data.brightness;

        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    server.on("/status", HTTP_GET, []() {
        JsonDocument doc;
        doc["uptime"]    = millis();
        doc["freeHeap"]  = ESP.getFreeHeap();
        doc["connected"] = (WiFi.status() == WL_CONNECTED);
        doc["ip"]        = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
        doc["apSSID"]    = "modular";
        doc["apIP"]      = WiFi.softAPIP().toString();
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    });

    server.on("/save", HTTP_POST, []() {
        if (server.hasArg("ssid"))   settings.data.wifiSSID     = server.arg("ssid");
        if (server.hasArg("pass") && !server.arg("pass").isEmpty())
                                     settings.data.wifiPassword = server.arg("pass");
        if (server.hasArg("owmkey")) settings.data.owmApiKey    = server.arg("owmkey");
        if (server.hasArg("city"))   settings.data.city         = server.arg("city");
        settings.save();
        server.send(200, "application/json", "{\"ok\":true}");

        bool restart = server.hasArg("restart") && server.arg("restart") == "1";
        if (restart) { delay(400); ESP.restart(); }
    });

    server.on("/brightness", HTTP_POST, []() {
        if (server.hasArg("value")) {
            int v = constrain(server.arg("value").toInt(), 0, 255);
            settings.data.brightness = v;
            settings.save();
            display.ssd1306_command(SSD1306_SETCONTRAST);
            display.ssd1306_command(v);
        }
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.on("/layout", HTTP_POST, []() {
        if (server.hasArg("clockX"))    settings.data.widgetClockX       = server.arg("clockX").toInt();
        if (server.hasArg("clockY"))    settings.data.widgetClockY       = server.arg("clockY").toInt();
        if (server.hasArg("clockSize")) settings.data.widgetClockSize    = constrain(server.arg("clockSize").toInt(), 1, 4);
        if (server.hasArg("clockVis"))  settings.data.widgetClockVisible = server.arg("clockVis") == "1";

        if (server.hasArg("dateX"))     settings.data.widgetDateX        = server.arg("dateX").toInt();
        if (server.hasArg("dateY"))     settings.data.widgetDateY        = server.arg("dateY").toInt();
        if (server.hasArg("dateSize"))  settings.data.widgetDateSize     = constrain(server.arg("dateSize").toInt(), 1, 4);
        if (server.hasArg("dateVis"))   settings.data.widgetDateVisible  = server.arg("dateVis") == "1";

        if (server.hasArg("weatherX"))    settings.data.widgetWeatherX       = server.arg("weatherX").toInt();
        if (server.hasArg("weatherY"))    settings.data.widgetWeatherY       = server.arg("weatherY").toInt();
        if (server.hasArg("weatherSize")) settings.data.widgetWeatherSize    = constrain(server.arg("weatherSize").toInt(), 1, 4);
        if (server.hasArg("weatherVis"))  settings.data.widgetWeatherVisible = server.arg("weatherVis") == "1";

        settings.save();
        server.send(200, "application/json", "{\"ok\":true}");
    });

    server.begin();
}

void setup() {
    Wire.begin();

    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.clearDisplay();
    display.display();
    display.setRotation(2);

    SPIFFS.begin(true);
    settings.load();

    display.ssd1306_command(SSD1306_SETCONTRAST);
    display.ssd1306_command(settings.data.brightness);

    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID);
    MDNS.begin(MDNS_NAME);

    setupWebServer();

    if (settings.isConfigured()) {
        WiFi.begin(settings.data.wifiSSID.c_str(), settings.data.wifiPassword.c_str());
        unsigned long t0 = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - t0 < 8000) { delay(150); }

        if (WiFi.status() == WL_CONNECTED) {
            timeClient.begin();
            buildTimeDate();
            fetchWeather();
        }
    }

    lastWeatherMs = lastClockMs = lastFadeMs = millis();

    if (!settings.isConfigured()) {
        drawOnboarding();
    }
}

void loop() {
    server.handleClient();

    if (!settings.isConfigured()) {
        drawOnboarding();
        return;
    }

    unsigned long now = millis();

    if (now - lastFadeMs    > 40)               { widgetUpdateWeatherFade(); lastFadeMs    = now; }
    if (now - lastClockMs   > CLOCK_UPDATE_MS)  { buildTimeDate();           lastClockMs   = now; }
    if (now - lastWeatherMs > WEATHER_UPDATE_MS){ fetchWeather();            lastWeatherMs = now; }

    drawMain();
}
