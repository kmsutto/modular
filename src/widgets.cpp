#include "widgets.h"
#include "config.h"

// -- variables
static String  wCurrent;
static String  tCurrent;
static String  wPending;
static String  tPending;
static uint8_t wAlpha       = 0;
static bool    wFadingOut   = false;

static const uint8_t ICO_SUN[5]   PROGMEM = {0x24,0x42,0x3C,0x42,0x24};
static const uint8_t ICO_CLOUD[5] PROGMEM = {0x38,0x7C,0xFE,0xFE,0x00};
static const uint8_t ICO_RAIN[5]  PROGMEM = {0x38,0x7C,0xFE,0x54,0x28};
static const uint8_t ICO_SNOW[5]  PROGMEM = {0x24,0x18,0xFF,0x18,0x24};

// -- weather icon
static void drawIcon(int x, int y, const String &cond) {
    const uint8_t *ico = ICO_SUN;
    String c = cond; c.toLowerCase();
    if      (c.indexOf("snow")    >= 0) ico = ICO_SNOW;
    else if (c.indexOf("rain")    >= 0 ||
             c.indexOf("drizzle") >= 0) ico = ICO_RAIN;
    else if (c.indexOf("cloud")   >= 0) ico = ICO_CLOUD;
    for (int r = 0; r < 5; r++) {
        uint8_t b = pgm_read_byte(&ico[r]);
        for (int c = 0; c < 8; c++)
            if (b & (0x80 >> c)) display.drawPixel(x + c, y + r, SSD1306_WHITE);
    }
}

// -- draw clock
void widgetDrawClock(int x, int y, const String &t) {
    display.setTextSize(3);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.print(t);
}

// -- draw date
void widgetDrawDate(int x, int y, const String &d) {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.print(d);
}

// -- draw weather
void widgetSetWeather(const String &condition, const String &temp) {
    if (condition == wCurrent && temp == tCurrent) return;
    wPending   = condition;
    tPending   = temp;
    wFadingOut = true;
}

void widgetDrawWeather(int x, int y) {
    if (wCurrent.isEmpty() || wAlpha < 32) return;
    
    drawIcon(x, y, wCurrent);
    
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    int textX = x + 10;
    display.setCursor(textX, y);
    display.print(tCurrent);
    
    display.drawCircle(textX + (tCurrent.length() * 6) + 1, y + 1, 1, SSD1306_WHITE);
    
    display.setCursor(textX + (tCurrent.length() * 6) + 4, y);
    display.print("C");
}

void widgetUpdateWeatherFade() {
    if (wFadingOut) {
        if (wAlpha >= WEATHER_FADE_STEPS) wAlpha -= WEATHER_FADE_STEPS;
        else {
            wAlpha     = 0;
            wCurrent   = wPending;
            tCurrent   = tPending;
            wFadingOut = false;
        }
    } else {
        if (wAlpha + WEATHER_FADE_STEPS < 255) wAlpha += WEATHER_FADE_STEPS;
        else wAlpha = 255;
    }
}

// -- edit mode
void widgetDrawEditorCursor(int x, int y, int w, int h) {
    display.drawRect(x - 2, y - 2, w + 4, h + 4, SSD1306_WHITE);
}
