#include "ads1115voltmeterwrap.h"

/* ---------------------------------
 * ADS1115 Analog to Digital Converter - Voltmeter
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C address default is 0x48
 * In the SensorInfo array (SENSORS[]), pin # refers to the analog pin on the ADS1115 that is being measured
 */

ADS1115VoltmeterWrap::ADS1115VoltmeterWrap(uint8_t ADCpin): pin(ADCpin)
{
    ads.setGain(GAIN);
    num_readings = 1;
}
void ADS1115VoltmeterWrap::init()
{
    Serial.println("ADC init on pin " + String(pin));
    labels = "Voltage[ADS]";
    if(!ads.begin())
    {
        Serial.println("ADC init failed.");
        disabled = true;
    }
}
// Just return voltage reading from analog in.
float ADS1115VoltmeterWrap::getReadingVoutVolts()
{
    return ads.computeVolts(ads.readADC_SingleEnded(pin));
}
// Return Vin voltage of voltage divider circuit
float ADS1115VoltmeterWrap::getReadingVinVolts()
{
    return getReadingVoutVolts() * (R1 + R2) / R2;
}
// Override this with any sensor-specific calculations
float ADS1115VoltmeterWrap::takeReading([[maybe_unused]] uint8_t reading_num)
{
    return getReadingVinVolts();
}
