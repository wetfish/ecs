#pragma once
#include <Adafruit_SSD1306.h>

/* -------------------------------
 * SSD1306 OLED Display
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * -0x3D for 128x64, 0x3C for 128x32 should be the correct address, but might need I2C scanner to check actual address
 * -128x32 size should fit 5ish lines of text
 */

class OledDisplay
{
    private:
    static const uint8_t SCREEN_WIDTH = 128; // OLED display width, in pixels
    static const uint8_t SCREEN_HEIGHT = 32; // OLED display height, in pixels
    static const uint8_t OLED_RESET = -1; // Reset pin # (or -1 if sharing Arduino reset pin)
    static const uint8_t SCREEN_ADDRESS = 0x3C; ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
    Adafruit_SSD1306 oled;

    public:
    OledDisplay(); // constructor
    void init();   // sets screens parameters
    void display();// displays given text TODO: take string as input probably?
};