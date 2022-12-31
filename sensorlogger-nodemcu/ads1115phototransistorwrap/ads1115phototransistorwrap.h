#pragma once
#include "ads1115voltmeterwrap.h"

/* ---------------------------------
 * ADS1115 ADC - Phototransistor Light Detector
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C address default is 0x48
 * In the SensorInfo array (SENSORS[]), pin # refers to the analog pin on the ADS1115 that is being measured
 * Takes one measurement - light level.
 */

class ADS1115PhototransistorWrap : public ADS1115VoltmeterWrap
{
	// Gain is a static variable in parent class ADS1115VoltmeterWrap.
	// GAIN_ONE corresponds to range of +-4.096V
	// GAIN_TWOTHIRDS  corresponds to range of +-6.144V 
	// GAIN_ONE recommended for 3.3V systems, GAIN_TWOTHIRDS needed for 5V
	public:
	using ADS1115VoltmeterWrap::ADS1115VoltmeterWrap;
	void init();
	float takeReading([[maybe_unused]] uint8_t reading_num);
	protected:
	// keep reading from going slightly over 100 or under 0
	float constrainReading(float calc_val);
};