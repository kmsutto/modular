#include "widgets.h"
#include "config.h"

static String  wCurrent;
static String  tCurrent;
static String  wPending;
static String  tPending;
static uint8_t wAlpha     = 0;
static bool    wFadingOut = false;

static const uint8_t ICO_SUN[5]   PROGMEM = {0x24, 0x42, 0x3C, 0x42, 0x24};
static const uint8_t ICO_CLOUD[5] PROGMEM = {0x38, 0x7C, 0xFE, 0xFE, 0x00};
static const uint8_t ICO_RAIN[5]  PROGMEM = {0x38, 0x7C, 0xFE, 0x54, 0x28};
static const uint8_t ICO_SNOW[5]  PROGMEM = {0x24, 0x18, 0xFF, 0x18, 0x24};

static void drawIcon(int x, int y, const String &cond) {
    const uint8_t *ico = ICO_SUN;
    String lc = cond;
    lc.toLowerCase();

    if      (lc.indexOf("snow")    >= 0) ico = ICO_SNOW;
    else if (lc.indexOf("rain")    >= 0 || lc.indexOf("drizzle") >= 0) ico = ICO_RAIN;
    else if (lc.indexOf("cloud")   >= 0) ico = ICO_CLOUD;

    for (int row = 0; row < 5; row++) {
        uint8_t b = pgm_read_byte(&ico[row]);
        for (int bit = 0; bit < 8; bit++)
            if (b & (0x80 >> bit))
                display.drawPixel(x + bit, y + row, SSD1306_WHITE);
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
        if (wAlpha >= 15) wAlpha -= 15;
        else {
            wAlpha = 0; wCurrent = wPending; tCurrent = tPending; wFadingOut = false;
        }
    } else {
        wAlpha = (wAlpha + 15 < 255) ? wAlpha + 15 : 255;
    }
}
