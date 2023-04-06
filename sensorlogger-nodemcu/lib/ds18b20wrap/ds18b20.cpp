#include <Arduino.h>
#include "ds18b20wrap.h"

/* ----------------
 * DS18B20 sensor: 
 * Reads temperature. Many identical sensors can be wired to the same pin.
 * num_readings does not need to be set manually. It's set in init() automatically.
 * pin_num = OneWire data pin number
 * If using an invalid address or there is a sensor missing, it will just read -196.6F
 */

String DS18B20Wrap::getLabels()
{
    String temp_labels = "";
    for(int i = 0; i < num_readings; i++)
    {
        temp_labels += "DS18 Temp(F)[" + DS18B20_addresses[i].label + "]";
        if(i < num_readings - 1)
        {
            temp_labels += ", ";
        }
    }
    return temp_labels;
}

DS18B20Wrap::DS18B20Wrap(uint8_t pin_num): onewire(pin_num) 
{
    //num_readings = NUM_DS18B20S;
    labels = getLabels();
}

void DS18B20Wrap::set_addresses(const DS18B20Addresses addresses[5], uint8_t num_addresses)
{
    for(int i = 0; i < num_addresses; i++)
    {
        // copy each address bit and label
        for(int j = 0; j < 8; j++)
        {
            DS18B20_addresses[i].address[j] = addresses[i].address[j];
        }
        DS18B20_addresses[i].label = addresses[i].label;
    }
}

void DS18B20Wrap::init()
{
    Serial.println("DS18B20 init: " + String(num_readings) + " sensors, label= " + labels);
    ds18b20.setOneWire(&onewire);
    ds18b20.begin();
}

float DS18B20Wrap::takeReading(uint8_t reading_num)
{
    if(reading_num == 0)
    {  // only request temps once for all readings
        ds18b20.requestTemperatures();
    }
    return ds18b20.getTempF(DS18B20_addresses[reading_num].address);
}
