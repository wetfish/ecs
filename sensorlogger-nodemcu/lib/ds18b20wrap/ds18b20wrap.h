#include "sensorwrap.h"
#include <DallasTemperature.h>

#ifndef DS18B20WRAP_H
#define DS18B20WRAP_H

struct DS18B20Addresses
{
	DeviceAddress address; 
	String label; // label to differentiate temperature readings
};

/* ----------------
 * DS18B20 sensor: 
 * Reads temperature. Many identical sensors can be wired to the same pin.
 * num_readings does not need to be set manually. It's set in init() automatically.
 * pin_num = OneWire data pin number
 * If using an invalid address or there is a sensor missing, it will just read -196.6F
 */
class DS18B20Wrap : public SensorWrap
{
	private:
	OneWire onewire;
	DallasTemperature ds18b20;
	String getLabels();
    DS18B20Addresses DS18B20_addresses[5]; // sets max number of ds18b20s to 5

	public:
	DS18B20Wrap(uint8_t pin_num);
    void set_addresses(const DS18B20Addresses addresses[5], uint8_t num_addresses);
	void init();
	float takeReading(uint8_t reading_num);
};

#endif