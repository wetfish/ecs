#include "ads1115anemometerwrap.h"

/* ---------------------
 * ADS1115 ADC - Anemometer
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * In the SensorInfo array (SENSORS[]), pin # refers to the analog pin on the ADS1115 that is being measured
 * Takes one reading: wind speed
 * //TODO: somehow test a range of values to see if our voltage to wind speed map is accurate
 * //TODO: maybe combine this with other ADS1115 readings
 */

// Gain is a static variable in parent class ADS1115VoltmeterWrap.
// GAIN_ONE corresponds to range of +-4.096V
// GAIN_TWO corresponds to range of +-2.048V - This should not be used along with other things
//  attached to the ADS115. They would probably give higher voltages (up to 3.3V likely).
// Anemometer should return max V of 2V.


void ADS1115AnemometerWrap::init()
{
    ads.setGain(GAIN);
    Serial.println("Windspd init");
    labels = "Wind Speed";
    if(!ads.begin())
    {
        Serial.println("ADC init failed.");
        disabled = true;
    }
}
// Might need some testing to confirm the ranges.
float ADS1115AnemometerWrap::takeReading([[maybe_unused]] uint8_t reading_num)
{
    // according to product page : https://www.adafruit.com/product/1733
    // voltage varies .4 - 2V and corresponds to wind speed of 0 - 32.4 m/s, with 9V supply to anemometer
    // assume linear interpolation until testing proves otherwise
    float Vmin = .4; // V for Volts
    float Vmax = 2.0;
    float vMin = 0; // v for velocity
    float vMax = 32.4;
    return (vMax - vMin) * (getReadingVoutVolts() - Vmin) / (Vmax - Vmin) + vMin;
}