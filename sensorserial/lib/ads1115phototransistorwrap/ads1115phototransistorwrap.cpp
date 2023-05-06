#include "ads1115phototransistorwrap.h"

/* ---------------------------------
 * ADS1115 ADC - Phototransistor Light Detector
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C address default is 0x48
 * In the SensorInfo array (SENSORS[]), pin # refers to the analog pin on the ADS1115 that is being measured
 * Takes one measurement - light level.
 */


// Gain is a static variable in parent class ADS1115VoltmeterWrap.
// GAIN_ONE corresponds to range of +-4.096V
// GAIN_TWOTHIRDS  corresponds to range of +-6.144V 
// GAIN_ONE recommended for 3.3V systems, GAIN_TWOTHIRDS needed for 5V

void ADS1115PhototransistorWrap::init()
{
    ads.setGain(GAIN);
    Serial.println("Light sensor init");
    labels = "Light (%)";
    if(!ads.begin())
    {
        Serial.println("ADC init failed.");
        disabled = true;
    }
}

float ADS1115PhototransistorWrap::takeReading([[maybe_unused]] uint8_t reading_num)
{
    // map voltage reading to light level (% of sensor's range)
    float Vmin = 0.01; // V for Volts
    float Vmax = 3.3;
    float lMin = 0; // light level - % of sensor's range. we have no way to calibrate this to known values currently.
    float lMax = 100;
    return constrainReading((lMax - lMin) * (getReadingVoutVolts() - Vmin) / (Vmax - Vmin) + lMin);
}

// keep reading from going slightly over 100 or under 0
float ADS1115PhototransistorWrap::constrainReading(float calc_val)
{
    if (calc_val > 100)
    {
        return 100.0f;
    }
    if (calc_val < 0)
    {
        return 0.0f;
    }
    return calc_val;
}