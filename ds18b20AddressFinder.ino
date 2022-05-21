/* Mostly taken from "Multiple.ino" example from 
 * https://www.milesburton.com/Dallas_Temperature_Control_Library
 * ------------------------------------------------------------------
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 * ------------------------------------------------------------------
 * 
 * To use, hook up one DS18B20 sensor at a time. Run program and use serial monitor to note 
 * address of each sensor tested. This will be needed when wiring multiple DS18B20 sensors
 * in parallel so we know which one is which.
 * 
 * DeviceAddress is an 8 element array of uint8_t, so address can be stored like so:
 * DeviceAddress sample_addr = {0x28, 0x63, 0x1B, 0x49, 0xF6, 0xAE, 0x3C, 0x88};
 */

#include <Arduino.h>
#include <DallasTemperature.h> // https://www.milesburton.com/Dallas_Temperature_Control_Library (for DS18B20)

// Pins printed on the NodeMCU board are not the same as the esp8266 GPIO pin number, which is what we actually need to use here
namespace NodeMcuPins
{
	enum : uint8_t
	{
		D0 = 16, D1 = 5, D2 = 4, D4 = 2,
		D5 = 14, D6 = 12, D7 = 13, D8 = 15,
		RX = 3, TX = 1, SD2 = 9, SD3 = 10
	};
}
// pick a pin for the data:
#define ONEWIRE_PIN NodeMcuPins::SD3

OneWire onewire(ONEWIRE_PIN);
DallasTemperature ds18b20(&onewire);
DeviceAddress sensor_address;
// function to print a device address
void printAddress(DeviceAddress deviceAddress);

void setup() 
{
  Serial.begin(115200);

  ds18b20.begin();
  Serial.println("number of devices: ");
  Serial.println(ds18b20.getDeviceCount(), DEC);

  if (!ds18b20.getAddress(sensor_address, 0)) 
  {
    while(1)
    {
      Serial.println("Unable to find address for Device 0");
      delay(5000);
    }
  }
}

void loop() 
{
  // show the address we found on the bus
  Serial.print("Device Address: ");
  printAddress(sensor_address);
  Serial.println();

  delay(5000);
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("{");
  for (uint8_t i = 0; i < 8; i++)
  {
    Serial.print("0x");
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
    // add comma if it's not the last byte
    if (i < 7)
    {
      Serial.print(", ");
    }
  }
  Serial.print("}");
}
