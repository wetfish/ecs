#include "ina219wrap.h"

/* ----------------------
 * INA219 voltmeter & ammeter
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C default address = 0x40, secondary is 0x41 (short A0 to VCC)
 * Vin+ and Vin- must be wired in series with current to be measured. Vin+ is high side, Vin- is low.
 * Can measure up to ~3.2A without breaking. Limiting component is shunt resistor. Sensor can 
 *  measure up to ~10-15A if resistor is swapped for one of even lower resistance.
 */

INA219Wrap::INA219Wrap(uint8_t i2cAddr) : ina(i2cAddr)
{
    sprintf(addr_hex, "%02X", i2cAddr);
    num_readings = 2; // Amps and Volts
    labels = "Power(mW)[0x" + String(addr_hex) + "], Voltage[0x" + String(addr_hex) + "]";
}

void INA219Wrap::init()
{
    Serial.println("INA init");
    if(!ina.begin())
    {
        Serial.println("INA init failed for address 0x" + String(addr_hex));
        disabled = true;
    }
}

float INA219Wrap::takeReading(uint8_t reading_num)
{
    switch (reading_num)
    {
        case 0:
            return ina.getPower_mW();
        case 1:
            return ina.getBusVoltage_V() + (ina.getShuntVoltage_mV() / 1000);
        default:
            return NAN;
    }
}