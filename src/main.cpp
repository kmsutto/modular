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

enum class AppMode     { CLOCK, EDITOR };
enum class EditorWidget { CLOCK, DATE, WEATHER };

AppMode      appMode          = AppMode::CLOCK;
EditorWidget editorSel        = EditorWidget::CLOCK;
bool         editorMoving     = false;

unsigned long lastWeatherMs   = 0;
unsigned long lastClockMs     = 0;
unsigned long lastFadeMs      = 0;
unsigned long btnPressMs      = 0;
bool          btnHeld         = false;
bool          longPressExecuted = false;

String cachedTime;
String cachedDate;
String cachedTemp = "0C";

static String readFile(const char *path) {
    File f = SPIFFS.open(path, "r");
    if (!f) return "";
    String s = f.readString();
    f.close();
    return s;
}

// -- time & data settings
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

// -- weather settings
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

// -- draw it all
static void drawMain() {
    display.clearDisplay();
    widgetDrawClock(settings.data.widgetClockX, settings.data.widgetClockY, cachedTime);
    widgetDrawDate(settings.data.widgetDateX, settings.data.widgetDateY, cachedDate);
    widgetDrawWeather(settings.data.widgetWeatherX, settings.data.widgetWeatherY);
    display.display();
}

// -- editor
static void drawEditor() {
    display.clearDisplay();

    widgetDrawClock(settings.data.widgetClockX, settings.data.widgetClockY, cachedTime);
    widgetDrawDate(settings.data.widgetDateX, settings.data.widgetDateY, cachedDate);
    widgetDrawWeather(settings.data.widgetWeatherX, settings.data.widgetWeatherY);

    int16_t x1, y1;
    uint16_t tw, th;
    int bx, by, bw, bh;

    switch (editorSel) {
        case EditorWidget::CLOCK:
            display.setTextSize(3);
            display.getTextBounds(cachedTime, settings.data.widgetClockX, settings.data.widgetClockY, &x1, &y1, &tw, &th);
            bx = x1 - 4; by = y1 - 4; bw = tw + 8; bh = th + 8;
            break;
        case EditorWidget::DATE:
            display.setTextSize(1);
            display.getTextBounds(cachedDate, settings.data.widgetDateX, settings.data.widgetDateY, &x1, &y1, &tw, &th);
            bx = x1 - 3; by = y1 - 3; bw = tw + 6; bh = th + 6;
            break;
        default:
            display.setTextSize(1);
            display.getTextBounds(cachedTemp, settings.data.widgetWeatherX + 18, settings.data.widgetWeatherY + 4, &x1, &y1, &tw, &th);
            bx = settings.data.widgetWeatherX - 2;
            by = settings.data.widgetWeatherY - 2;
            bw = (x1 + tw) - bx + 2;
            bh = 16 + 4;
            break;
    }

    display.drawRoundRect(bx, by, bw, bh, 3, SSD1306_WHITE);
    display.display();
}

// -- joy preferences
static void handleJoystick() {
    int jx = analogRead(JOY_X_PIN) - JOY_CENTER;
    int jy = analogRead(JOY_Y_PIN) - JOY_CENTER;
    bool pressed = (digitalRead(JOY_BTN_PIN) == LOW);
    unsigned long now = millis();

    if (pressed) {
        if (!btnHeld) {
            btnPressMs = now;
            btnHeld = true;
            longPressExecuted = false;
        } else if (!longPressExecuted && (now - btnPressMs >= JOY_LONG_PRESS_MS)) {
            if (appMode == AppMode::CLOCK) {
                appMode = AppMode::EDITOR;
                editorSel = EditorWidget::CLOCK;
                editorMoving = false;
            } else if (appMode == AppMode::EDITOR) {
                settings.save();
                appMode = AppMode::CLOCK;
            }
            longPressExecuted = true;
        }
    } else {
        if (btnHeld) {
            if (!longPressExecuted && appMode == AppMode::EDITOR) {
                editorMoving = !editorMoving;
            }
            btnHeld = false;
        }
    }

    if (appMode != AppMode::EDITOR) return;

    static unsigned long lastMove = 0;
    if (now - lastMove < 120) return;
    bool moved = false;

    if (!editorMoving) {
        if (abs(jy) > JOY_DEAD_ZONE) {
            int step = (jy > 0) ? 1 : 2;
            editorSel = (EditorWidget)(((int)editorSel + step) % 3);
            moved = true;
        }
    } else {
        int *wx = nullptr, *wy = nullptr;
        if      (editorSel == EditorWidget::CLOCK)   { wx = &settings.data.widgetClockX;   wy = &settings.data.widgetClockY; }
        else if (editorSel == EditorWidget::DATE)    { wx = &settings.data.widgetDateX;    wy = &settings.data.widgetDateY; }
        else                                          { wx = &settings.data.widgetWeatherX; wy = &settings.data.widgetWeatherY; }

        if (abs(jx) > JOY_DEAD_ZONE) { *wx = constrain(*wx + (jx > 0 ? 2 : -2), 0, SCREEN_WIDTH  - 20); moved = true; }
        if (abs(jy) > JOY_DEAD_ZONE) { *wy = constrain(*wy + (jy > 0 ? 2 : -2), 0, SCREEN_HEIGHT - 10); moved = true; }
    }
    if (moved) lastMove = now;
}

// -- web server
static void setupWebServer() {
    server.on("/", HTTP_GET, []() {
        String html = readFile("/index.html");
        server.send(200, "text/html", html.isEmpty() ? "Error loading page" : html);
    });

    server.on("/config", HTTP_GET, []() {
        JsonDocument doc;
        doc["ssid"]           = settings.data.wifiSSID;
        doc["city"]           = settings.data.city;
        doc["owmkey"]         = settings.data.owmApiKey;
        doc["connected"]      = (WiFi.status() == WL_CONNECTED);
        doc["ip"]             = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "";
        // doc["clockX"]         = settings.data.widgetClockX;
        // doc["clockY"]         = settings.data.widgetClockY;
        // doc["dateX"]          = settings.data.widgetDateX;
        // doc["dateY"]          = settings.data.widgetDateY;
        // doc["weatherX"]       = settings.data.widgetWeatherX;
        // doc["weatherY"]       = settings.data.widgetWeatherY;
        // doc["cachedTime"]     = cachedTime;
        // doc["cachedDate"]     = cachedDate;
        // doc["cachedTemp"]     = cachedTemp;
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
        if (server.hasArg("ssid"))   settings.data.wifiSSID      = server.arg("ssid");
        if (server.hasArg("pass") && !server.arg("pass").isEmpty())
                                     settings.data.wifiPassword  = server.arg("pass");
        if (server.hasArg("owmkey")) settings.data.owmApiKey     = server.arg("owmkey");
        if (server.hasArg("city"))   settings.data.city          = server.arg("city");
        settings.save();
        server.send(200, "application/json", "{\"ok\":true}");

        bool restart = server.hasArg("restart") && server.arg("restart") == "1";
        if (restart) {
            delay(400);
            ESP.restart();
        }
    });

    server.begin();
}

// -- main
void setup() {
    Wire.begin();
    
    pinMode(JOY_BTN_PIN, INPUT_PULLUP);

    display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    display.clearDisplay();
    display.display();
    display.setRotation(2);

    SPIFFS.begin(true);
    settings.load();

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
}

void loop() {
    server.handleClient();

    unsigned long now = millis();
    handleJoystick();

    if (now - lastFadeMs    > 40)              { widgetUpdateWeatherFade(); lastFadeMs    = now; }
    if (now - lastClockMs   > CLOCK_UPDATE_MS) { buildTimeDate();           lastClockMs   = now; }
    if (now - lastWeatherMs > WEATHER_UPDATE_MS) { fetchWeather();          lastWeatherMs = now; }

    if (appMode == AppMode::EDITOR) drawEditor();
    else                            drawMain();
}
