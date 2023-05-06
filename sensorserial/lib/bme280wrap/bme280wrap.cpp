#include "bme280wrap.h"

/* -------------------------------
 * BME280
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * 0x76 should be the correct address, but might need I2C scanner to check actual address
 * //TODO: url of i2c scanner example
 * In the SensorInfo array (SENSORS[]), pin # doesn't matter for this sensor. Put any int.
 */

BME280Wrap::BME280Wrap()
{
    num_readings = IS_TAKING_P + IS_TAKING_RH + IS_TAKING_TEMP;
    labels = "";
    if (IS_TAKING_P)
    {
        labels += "BME280 P(hPa), ";
    }
    if (IS_TAKING_RH)
    {
        labels += "BME280 RH(%), ";
    }
    if (IS_TAKING_TEMP)
    {
        labels += "BME280 Temp(F), ";
    }
    if (num_readings && labels.endsWith(", "))
    {
        labels.remove(labels.length() - 2, 2);
    }
}

void BME280Wrap::init()
{
    Serial.println("BME280 init");
    if(!bme.begin(I2C_ADDR))
    {
        Serial.println("Error: bme280 failed");
        disabled = true;
        //while(1){} // this will reset the nodeMCU. it doesn't like blocking code. to make this actually block forever, add a yield()
    }
    // sampling settings:
    // forced mode only takes readings as needed, no oversampling, and we don't need filter
    // for any measurements not being taken, set to SAMPLING_X0
    bme.setSampling(Adafruit_BME280::MODE_FORCED,
                Adafruit_BME280::SAMPLING_X1, // temperature
                Adafruit_BME280::SAMPLING_X1, // pressure
                Adafruit_BME280::SAMPLING_X1, // humidity
                Adafruit_BME280::FILTER_OFF   );
}

float BME280Wrap::takeReading(uint8_t reading_num)
{
    bme.takeForcedMeasurement();
    switch (reading_num)
    {
    case 0:
        return bme.readPressure() / 100.0f; // taken in Pa, converted to hPa (millibars)
    case 1:
        return bme.readHumidity();
    case 2:
        return bme.readTemperature() * 9.0f / 5.0f + 32; // taken in C, converted to F
    default:
        return NAN;
    }
}
