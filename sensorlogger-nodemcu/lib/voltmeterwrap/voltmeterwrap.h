#include "sensorwrap.h"


#ifndef VOLTMETERWRAP_H
#define VOLTMETERWRAP_H

/* ------------------------
 * Analog Voltmeter
 * Reads volts (or any analog sensor) from onboard analog pin
 * On NodeMCU, ADC is 10 bit precision from 0 - 3.3V - (onboard v-divider: 220kOhm -- 100kOhm)
 * On NodeMCU, the only analog pin is A0
 * For an input voltage range of 0 - 3.3V(or 5.0V for some boards), scale is set to default of 1.0
 * If using a voltage divider to measure a different range of voltages, use appropriate scale
 */
class VoltmeterWrap : public SensorWrap
{	
	const float INPUT_MAX_VOLTAGE = 3.3; // 3.3 for nodemcu. 5.0 for many arduinos, others
	uint8_t analog_pin;
	float scale;

	public:
	VoltmeterWrap(uint8_t analogPin, float scale = 2.0f) : analog_pin(analogPin), scale(scale) {}

	void init();
	float takeReading([[maybe_unused]] uint8_t reading_num);
};

#endif