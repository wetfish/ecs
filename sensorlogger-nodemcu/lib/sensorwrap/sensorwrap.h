#ifndef SENSORWRAP_H
#define SENSORWRAP_H

#include "Arduino.h"


// When adding new sensors, just make a wrapper that has the following variables and methods:
#define MAX_READINGS 10 // this should be enough for stuff like the ina219. increase if needed
class SensorWrap
{
	public:
	bool disabled = false; // disables calls to hardware
  	uint8_t num_readings = 1; // how many readings to take from this sensor (a DHT22 has 2: temperature and humidity)
	float last_reading[MAX_READINGS]; // keep track of last reading in case we need to report it
	String labels; // label(s) for the data gathered from sensor
	virtual void init(){} // whatever needs to go into setup()
	float getReading(uint8_t reading_num);
	virtual float takeReading(uint8_t reading_num);// return measured value (NAN if no reading taken)
};

#endif
