#pragma once
#include "ads1115voltmeterwrap.h"

/* ---------------------
 * ADS1115 ADC - Anemometer
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * In the SensorInfo array (SENSORS[]), pin # refers to the analog pin on the ADS1115 that is being measured
 * Takes one reading: wind speed
 * //TODO: somehow test a range of values to see if our voltage to wind speed map is accurate
 * //TODO: maybe combine this with other ADS1115 readings
 */
class ADS1115AnemometerWrap : public ADS1115VoltmeterWrap
{
	// Gain is a static variable in parent class ADS1115VoltmeterWrap.
	// GAIN_ONE corresponds to range of +-4.096V
	// GAIN_TWO corresponds to range of +-2.048V - This should not be used along with other things
	//  attached to the ADS115. They would probably give higher voltages (up to 3.3V likely).
	// Anemometer should return max V of 2V.
	public:
	using ADS1115VoltmeterWrap::ADS1115VoltmeterWrap;
	void init();
	float takeReading([[maybe_unused]] uint8_t reading_num);
};