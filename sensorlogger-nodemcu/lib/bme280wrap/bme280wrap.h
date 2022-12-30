#include "sensorwrap.h"
#include <Adafruit_BME280.h> // https://github.com/adafruit/Adafruit_BME280_Library 

#ifndef BME280WRAP_H
#define BME280WRAP_H

/* -------------------------------
 * BME280
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * 0x76 should be the correct address, but might need I2C scanner to check actual address
 * //TODO: url of i2c scanner example
 * In the SensorInfo array (SENSORS[]), pin # doesn't matter for this sensor. Put any int.
 */
class BME280Wrap : public SensorWrap
{
	private:
	// might need an i2c scanner if this address is incorrect
	const uint8_t I2C_ADDR = 0x76;
	// set which values to measure - reorder the switch statement if there are any true after a false
	const bool IS_TAKING_P = true;
	const bool IS_TAKING_RH = true;
	const bool IS_TAKING_TEMP = true;
	Adafruit_BME280 bme; // this is I2C mode, constructor takes CS pin as argument for SPI mode
	public:
	BME280Wrap();

	void init();

	float takeReading(uint8_t reading_num);
};

#endif