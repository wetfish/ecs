#ifndef ADS1115VOLTMETERWRAP_H
#define ADS1115VOLTMETERWRAP_H

#include "sensorwrap.h"
#include <Adafruit_ADS1X15.h> // https://github.com/adafruit/Adafruit_ADS1X15 (ADC board)

/* ---------------------------------
 * ADS1115 Analog to Digital Converter - Voltmeter
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C address default is 0x48
 * In the SensorInfo array (SENSORS[]), pin # refers to the analog pin on the ADS1115 that is being measured
 */
class ADS1115VoltmeterWrap : public SensorWrap
{
	protected:
	// Set Gain here. It determines voltage input range.
	// Things might go badly if you exceed range. Maybe pick a larger range until
	//  you're certain of the full input range.
	//    GAIN           Range   |     GAIN         Range
	// GAIN_TWOTHIRDS: +-6.144V  |  GAIN_FOUR:    +-1.024V
	// GAIN_ONE:       +-4.096V  |  GAIN_EIGHT:   +-0.512V
	// GAIN_TWO:       +-2.048V  |  GAIN_SIXTEEN: +-0.256V
	static const adsGain_t GAIN = GAIN_ONE;
	// Set voltage divider resistors here.
	// If not using voltage divider, set R1 to 0 and R2 to any positive value
	const float R1 = 0;
	const float R2 = 1;

	// Addresses depend on what ADDR pin in shorted to (default GND)
	// GND: 0x48   |  SDA: 0x4A
	// VDD: 0x49   |  SCL: 0x4B
	Adafruit_ADS1115 ads;
	uint8_t pin;
	public:
	ADS1115VoltmeterWrap(uint8_t ADCpin);
	virtual void init();
	// Just return voltage reading from analog in.
	float getReadingVoutVolts();
	// Return Vin voltage of voltage divider circuit
	float getReadingVinVolts();
	// Override this with any sensor-specific calculations
	virtual float takeReading([[maybe_unused]] uint8_t reading_num);
};
#endif