#pragma once
#include "sensorwrap.h"
#include <Adafruit_INA219.h> // https://github.com/adafruit/Adafruit_INA219 (Volt/Ammeter)

/* ----------------------
 * INA219 voltmeter & ammeter
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C default address = 0x40, secondary is 0x41 (short A0 to VCC)
 * Vin+ and Vin- must be wired in series with current to be measured. Vin+ is high side, Vin- is low.
 * Can measure up to ~3.2A without breaking. Limiting component is shunt resistor. Sensor can 
 *  measure up to ~10-15A if resistor is swapped for one of even lower resistance.
 */
class INA219Wrap : public SensorWrap
{
	private:
	Adafruit_INA219 ina;
	char addr_hex[2];

	public:
	INA219Wrap(uint8_t i2cAddr);
	void init();
	float takeReading(uint8_t reading_num);
};