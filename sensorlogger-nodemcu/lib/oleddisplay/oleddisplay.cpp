#include "oleddisplay.h"

/* -------------------------------
 * SSD1306 OLED Display
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * -0x3D for 128x64, 0x3C for 128x32 should be the correct address, but might need I2C scanner to check actual address
 * -128x32 size should fit 5ish lines of text
 */

OledDisplay::OledDisplay() : oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

void OledDisplay::init()
{
    oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    oled.clearDisplay();
    oled.setTextSize(1);      // Normal 1:1 pixel scale
    oled.setTextColor(SSD1306_WHITE); // Draw white text
    oled.setCursor(0, 0);     // Start at top-left corner
    oled.cp437(true);         // Use full 256 char 'Code Page 437' font
}

void OledDisplay::display()
{

}