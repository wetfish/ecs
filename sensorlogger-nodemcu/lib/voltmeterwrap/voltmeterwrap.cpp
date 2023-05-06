#include "voltmeterwrap.h"

/* ------------------------
 * Analog Voltmeter
 * Reads volts (or any analog sensor) from onboard analog pin
 * On NodeMCU, ADC is 10 bit precision from 0 - 3.3V - (onboard v-divider: 220kOhm -- 100kOhm)
 * On NodeMCU, the only analog pin is A0
 * For an input voltage range of 0 - 3.3V(or 5.0V for some boards), scale is set to default of 1.0
 * If using a voltage divider to measure a different range of voltages, use appropriate scale
 */

void VoltmeterWrap::init()
{
    pinMode(analog_pin, INPUT);
    num_readings = 1;
    labels = "Voltage[ADC]";
}
float VoltmeterWrap::takeReading([[maybe_unused]] uint8_t reading_num)
{
    int reading = analogRead(analog_pin); // returns value from 0-1023
    float V = scale * (reading * INPUT_MAX_VOLTAGE) / 1023.0f; 
    return V;
}