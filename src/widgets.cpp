#include "widgets.h"
#include "config.h"

static String  wCurrent;
static String  tCurrent;
static String  wPending;
static String  tPending;
static uint8_t wAlpha     = 0;
static bool    wFadingOut = false;

static const uint8_t ICO_SUN[5]   PROGMEM = {0x24,0x42,0x3C,0x42,0x24};
static const uint8_t ICO_CLOUD[5] PROGMEM = {0x38,0x7C,0xFE,0xFE,0x00};
static const uint8_t ICO_RAIN[5]  PROGMEM = {0x38,0x7C,0xFE,0x54,0x28};
static const uint8_t ICO_SNOW[5]  PROGMEM = {0x24,0x18,0xFF,0x18,0x24};

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

void widgetDrawClock(int x, int y, const String &t, int size) {
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.print(t);
}

void widgetDrawDate(int x, int y, const String &d, int size) {
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(x, y);
    display.print(d);
}

void widgetSetWeather(const String &condition, const String &temp) {
    if (condition == wCurrent && temp == tCurrent) return;
    wPending   = condition;
    tPending   = temp;
    wFadingOut = true;
}

void widgetDrawWeather(int x, int y, int size) {
    if (wCurrent.isEmpty() || wAlpha < 32) return;

    drawIcon(x, y, wCurrent);

    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);

    int textX = x + 10;
    display.setCursor(textX, y);
    display.print(tCurrent);

    int dotX = textX + (tCurrent.length() * 6 * size) + 1;
    display.drawCircle(dotX, y + 1, 1, SSD1306_WHITE);

    display.setCursor(dotX + 3, y);
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
