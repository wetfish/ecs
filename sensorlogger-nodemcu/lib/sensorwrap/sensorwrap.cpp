#include "sensorwrap.h"
#include <Arduino.h>

float SensorWrap::getReading(uint8_t reading_num)
{
    if (disabled) // if sensor init failed, don't even try to read from it from now on
    {
        return NAN;
    }
    else
    {
        return takeReading(reading_num);
    }
} 

float SensorWrap::takeReading(uint8_t reading_num){return NAN;} // return measured value (NAN if no reading taken)
