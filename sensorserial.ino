/* sensorSerial.cpp (or .ino)
 *
 * Establishes link to a PC and sends sensor data via USB
 * when PC program calls for a reading. 
 * 
 */

#include <SPI.h>
#include <SD.h> // http://www.arduino.cc/en/Reference/SD
#include <DallasTemperature.h>  // https://www.milesburton.com/Dallas_Temperature_Control_Library (for DS18B20)
#include <Adafruit_BME280.h> // https://github.com/adafruit/Adafruit_BME280_Library 
#include <Adafruit_ADS1X15.h> // https://github.com/adafruit/Adafruit_ADS1X15 (ADC board)
#include <Adafruit_INA219.h> // https://github.com/adafruit/Adafruit_INA219 (Volt/Ammeter)

// These are the valid sensor models than can be used.
enum SensorModels
{/*dht11, dht22,*/ ds18b20, voltmeter, bme280, ads1115, ads1115anemometer, ads1115phototransistor, ina219};
struct SensorInfo
{
	SensorModels model;
	uint8_t pin; // data pin of that sensor - for I2C devices, this usually is arbitrary
};
struct DS18B20Addresses
{
	DeviceAddress address; 
	String label; // label to differentiate temperature readings
};
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
namespace Ads1115Pins
{
	enum : uint8_t
	{
		A0 = 0, A1 = 1, A2 = 2, A3 = 3
	};
}
//----------------------------------------------------------
// Change polling interval and sensor types here
const unsigned long POLL_INTERVAL = 3000; // in milliseconds
const SensorInfo SENSORS[] = 
{// {sensor model, pin # (or I2C address for INA219)}
	//{ina219, 0x40}, 
	//{ina219, 0x41}, 
	//{ads1115, Ads1115Pins::A0}, 
	//{bme280, 0},
	//{ds18b20, NodeMcuPins::SD3},
	//{ads1115phototransistor, Ads1115Pins::A0},
	//{ads1115anemometer, Ads1115Pins::A1},
  {voltmeter,A0}
};

// --- DS18B20 Addresses ---
// List sensor addresses here. Get them individually with ds18b20AddressFinder.ino or similar
// Labels should be where the sensor is located or something. Or some identifying factor.
const DS18B20Addresses DS18B20_ADDRESSES[] = {
		{{0x28, 0x63, 0x1B, 0x49, 0xF6, 0xAE, 0x3C, 0x88}, "MCU"}, // measuring electronics enclosure temp,has white heatshrink
		{{0x28, 0xDC, 0xA0, 0x49, 0xF6, 0xEB, 0x3C, 0xF0}, "Batt"}, // measuring battery enclosure temp, has white heatshrink
		//{{0x28, 0x6A, 0x64, 0x49, 0xF6, 0xBE, 0x3C, 0xF0}, "red"} // has red heatshrink
		};

uint8_t NUM_SENSORS = sizeof(SENSORS) / sizeof(SENSORS[0]);
uint8_t NUM_DS18B20S = sizeof(DS18B20_ADDRESSES) / sizeof(DS18B20_ADDRESSES[0]);

//----------------------------------------------------------
// Other Pin selection
// Pins D5 (SCK), D6(MISO), D7(MOSI), and D8(CS) on NodeMCU are required for the SPI for the SD card reader, so don't use them.
// On NodeMCU, pin D1(SCL) and D2(SDA) for I2C communication
const uint8_t CS_PIN = NodeMcuPins::D8;// cs pin for sd card
const uint8_t LED_PIN = LED_BUILTIN_AUX; // led pin for indicator. not required.

// When adding new sensors, just make a wrapper that has the following variables and methods:
class SensorWrap
{
	public:
	uint8_t num_readings = 1; // how many readings to take from this sensor (a DHT22 has 2: temperature and humidity)
	String labels; // label(s) for the data gathered from sensor
	virtual void init(){} // whatever needs to go into setup()
	virtual float getReading(uint8_t reading_num){return NAN;} // return measured value (NAN if no reading taken)
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

	String getLabels()
	{
		String temp_labels = "";
		for(int i = 0; i < num_readings; i++)
		{
			temp_labels += "DS18 Temp(F)[" + DS18B20_ADDRESSES[i].label + "]";
			if(i < num_readings - 1)
			{
				temp_labels += ", ";
			}
		}
		return temp_labels;
	}

	public:
	DS18B20Wrap(uint8_t pin_num): onewire(pin_num) 
	{
		num_readings = NUM_DS18B20S;
		labels = getLabels();
	}

	void init()
	{
		Serial.println("DS18B20 init: " + String(num_readings) + " sensors");
		ds18b20.setOneWire(&onewire);
		ds18b20.begin();
	}

	float getReading(uint8_t reading_num)
	{
		if(reading_num == 0)
		{  // only request temps once for all readings
			ds18b20.requestTemperatures();
		}
		return ds18b20.getTempF(DS18B20_ADDRESSES[reading_num].address);
	}
};


/* ------------------------
 * Analog Voltmeter
 * Reads volts (or any analog sensor) from onboard analog pin
 * On NodeMCU, ADC is 10 bit precision from 0 - 3.3V - (onboard v-divider: 220kOhm -- 100kOhm)
 * On NodeMCU, the only analog pin is A0
 * For an input voltage range of 0 - 3.3V(or 5.0V for some boards), scale is set to default of 1.0
 * If using a voltage divider to measure a different range of voltages, use appropriate scale
 */
class VoltmeterWrap : public SensorWrap
{	
	const float INPUT_MAX_VOLTAGE = 3.3; // 3.3 for nodemcu. 5.0 for many arduinos, others
	uint8_t analog_pin;
	float scale;

	public:
	VoltmeterWrap(uint8_t analogPin, float scale = 1.0f) : analog_pin(analogPin), scale(scale) {}

	void init()
	{
		pinMode(analog_pin, INPUT);
		num_readings = 1;
		labels = "Voltage[ADC]";
	}
	float getReading([[maybe_unused]] uint8_t reading_num)
	{
		int reading = analogRead(analog_pin); // returns value from 0-1023
		float V = scale * (reading * INPUT_MAX_VOLTAGE) / 1023.0f; 
		return V;
	}
};



/* -------------------------------
 * BME280
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * 0x76 should be the correct address, but might need I2C scanner to check actual address
 * //TODO: url of i2c scanner example
 * In the SensorInfo array (SENSORS[]), pin # doesn't matter for this sensor. Put any int.
 */
class BME280Wrap : public SensorWrap
{
	private:
	// might need an i2c scanner if this address is incorrect
	const uint8_t I2C_ADDR = 0x76;
	// set which values to measure - reorder the switch statement if there are any true after a false
	const bool IS_TAKING_P = true;
	const bool IS_TAKING_RH = true;
	const bool IS_TAKING_TEMP = true;
	Adafruit_BME280 bme; // this is I2C mode, constructor takes CS pin as argument for SPI mode
	public:
	BME280Wrap()
	{
		num_readings = IS_TAKING_P + IS_TAKING_RH + IS_TAKING_TEMP;
		labels = "";
		if (IS_TAKING_P)
		{
			labels += "BME280 P(hPa), ";
		}
		if (IS_TAKING_RH)
		{
			labels += "BME280 RH(%), ";
		}
		if (IS_TAKING_TEMP)
		{
			labels += "BME280 Temp(F), ";
		}
		if (num_readings && labels.endsWith(", "))
		{
			labels.remove(labels.length() - 2, 2);
		}

	}

	void init()
	{
		Serial.println("BME280 init");
		if(!bme.begin(I2C_ADDR))
		{
			Serial.println("Error: bme280 failed");
			while(1){}
		}
		// sampling settings:
		// forced mode only takes readings as needed, no oversampling, and we don't need filter
		// for any measurements not being taken, set to SAMPLING_X0
		bme.setSampling(Adafruit_BME280::MODE_FORCED,
                    Adafruit_BME280::SAMPLING_X1, // temperature
                    Adafruit_BME280::SAMPLING_X1, // pressure
                    Adafruit_BME280::SAMPLING_X1, // humidity
                    Adafruit_BME280::FILTER_OFF   );
		
	}

	float getReading(uint8_t reading_num)
	{
		bme.takeForcedMeasurement();
		switch (reading_num)
		{
		case 0:
			return bme.readPressure() / 100.0f; // taken in Pa, converted to hPa (millibars)
		case 1:
			return bme.readHumidity();
		case 2:
			return bme.readTemperature() * 9.0f / 5.0f + 32; // taken in C, converted to F
		default:
			return NAN;
		}
	}
};

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
	ADS1115VoltmeterWrap(uint8_t ADCpin): pin(ADCpin)
	{
		ads.setGain(GAIN);
		num_readings = 1;
	}
	virtual void init()
	{
		Serial.println("ADC init on pin " + String(pin));
		labels = "Voltage[ADS]";
		ads.begin();
	}
	// Just return voltage reading from analog in.
	float getReadingVoutVolts()
	{
		return ads.computeVolts(ads.readADC_SingleEnded(pin));
	}
	// Return Vin voltage of voltage divider circuit
	float getReadingVinVolts()
	{
		return getReadingVoutVolts() * (R1 + R2) / R2;
	}
	// Override this with any sensor-specific calculations
	virtual float getReading([[maybe_unused]] uint8_t reading_num)
	{
		return getReadingVinVolts();
	}
};

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
	void init()
	{
		ads.setGain(GAIN);
		Serial.println("Windspd init");
		labels = "Wind Speed";
		ads.begin();
	}
	// Might need some testing to confirm the ranges.
	float getReading([[maybe_unused]] uint8_t reading_num)
	{
		// according to product page : https://www.adafruit.com/product/1733
		// voltage varies .4 - 2V and corresponds to wind speed of 0 - 32.4 m/s, with 9V supply to anemometer
		// assume linear interpolation until testing proves otherwise
		float Vmin = .4; // V for Volts
		float Vmax = 2.0;
		float vMin = 0; // v for velocity
		float vMax = 32.4;
		return (vMax - vMin) * (getReadingVoutVolts() - Vmin) / (Vmax - Vmin) + vMin;
	}
};

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
	void init()
	{
		ads.setGain(GAIN);
		Serial.println("Light sensor init");
		labels = "Light (%)";
		ads.begin();
	}

	float getReading([[maybe_unused]] uint8_t reading_num)
	{
		// map voltage reading to light level (% of sensor's range)
		float Vmin = 0.01; // V for Volts
		float Vmax = 3.3;
		float lMin = 0; // light level - % of sensor's range. we have no way to calibrate this to known values currently.
		float lMax = 100;
		return constrainReading((lMax - lMin) * (getReadingVoutVolts() - Vmin) / (Vmax - Vmin) + lMin);
	}

	protected:
	// keep reading from going slightly over 100 or under 0
	float constrainReading(float calc_val)
	{
		if (calc_val > 100)
		{
			return 100.0f;
		}
		if (calc_val < 0)
		{
			return 0.0f;
		}
		return calc_val;
	}
};

/* ----------------------
 * INA219 voltmeter & ammeter
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C default address = 0x40, secondary is 0x41 (short A0 to VCC)
 * Vin+ and Vin- must be wired in series with current to be measured. Vin+ is high side, Vin- is low.
 * Can measure up to ~3.2A without breaking. Limiting component is shunt resistor. Sensor can 
 *  measure up to ~10-15A if resistor is swapped for one of even lower resistance.
 */
class INA219Wrap : public SensorWrap
{
	private:
	Adafruit_INA219 ina;
	char addr_hex[2];

	public:
	INA219Wrap(uint8_t i2cAddr) : ina(i2cAddr)
	{
		sprintf(addr_hex, "%02X", i2cAddr);
		num_readings = 2; // Amps and Volts
		labels = "Power(mW)[0x" + String(addr_hex) + "], Voltage[0x" + String(addr_hex) + "]";
	}

	void init()
	{
		Serial.println("INA init");
		if(!ina.begin())
		{
			Serial.println("INA init failed for address 0x" + String(addr_hex));
		}
	}

	float getReading(uint8_t reading_num)
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

};

/* --------------
 * Linked list for storing list of sensors.
 * TODO: maybe get rid of list_size. Doesn't look like it's being used. or maybe just use it.
 */
template <typename T>
class LinkedList
{
	
	public:

	struct Node
	{
		T sensor;
		Node* next;
	};

	Node* head;
	uint8_t list_size;

	LinkedList<T>()
	{
		head = nullptr;
		list_size = 0;
	}

	
	Node* get_next_node(Node* curr_node)
	{
		return curr_node->next;
	}

	// add node to end of list
	void push_back(T newData)
	{
		Node* new_node = get_new_node(newData);
		if (head == nullptr)
		{
			head = new_node;
		}
		else
		{
			Node* curr_node = head;
			while (curr_node->next)
			{
				curr_node = curr_node->next;
			}
			curr_node->next = new_node;
			list_size++;
		}
	}

	private:

	// creates new node
	Node* get_new_node(T newSensor)
	{
		Node* newNode = new Node;
		newNode->sensor = newSensor;
		newNode->next = nullptr;

		return newNode;
	}
	
};

/* =================================
 * DataLogger
 * Makes list of sensors, writes data from each of them to a new file based on the given filename base.
 * Uses LinkedList class and all of the SensorWrap children.
 * //TODO: possibly restructure this so it's easier to actually access and do stuff with any 
 *  of the sensor values.
 */
class DataLogger
{
	String log_fn;
	String column_labels;
	uint8_t led_pin;
	unsigned long poll_interval;
	unsigned long last_poll;
	unsigned long poll_start;
	bool is_first_poll;
	LinkedList<SensorWrap*>* sensor_list = new LinkedList<SensorWrap*>();
	bool did_err_msg = false;
	
	public:
	/* 
	 * logFn- filename should be short so as to conform to 8.3 short format (8 characters + '.' + 3 characters)
	 * pollInterval- time between sensor polls in milliseconds	 *
	 */
	DataLogger(uint8_t LEDpinNo, unsigned long pollInterval)
	{
		led_pin = LEDpinNo;
		poll_interval = pollInterval;
		last_poll = 0;
		column_labels = "Time(s)";
		is_first_poll = true;
	}
	
	// initializer. this is run in setup()
	void init_sensors(const SensorInfo sensors[], uint8_t& num_sensors)
	{
	
		list_sensors(sensors, num_sensors);
		// iterate through list of sensors to get the column labels from all of them
		LinkedList<SensorWrap*>::Node* current_sensor = sensor_list->head;
		while(current_sensor)
		{
			current_sensor->sensor->init();
			column_labels += ", " + current_sensor->sensor->labels;
			current_sensor = sensor_list->get_next_node(current_sensor);
		}
		// wait for serial connection
    	wait_for_serial();
		// show what each column means
		Serial.println(column_labels);
	}

	// takes readings and writes to file
	void update()
	{
		if(get_command())
		{
			String sensor_data = read_from_sensors();
		}
	}

	void wait_for_serial()
	{
		while(true)
		{
			Serial.println("ecs");
			delay(300);
			if(Serial.available())
			{
				if(Serial.readString() == "go")
				{
					return;
				}
			}
		}
	}

	private:
	
	// populate list
	// add another case for each new sensor class added 
	void list_sensors(const SensorInfo sensors[], uint8_t& num_sensors)
	{
		for (int i = 0; i < num_sensors; i++)
		{
			switch (sensors[i].model)
			{
			case ds18b20:
				sensor_list->push_back(new DS18B20Wrap(sensors[i].pin));
				break;
			case voltmeter:
				sensor_list->push_back(new VoltmeterWrap(sensors[i].pin));
				break;
			case bme280:
				sensor_list->push_back(new BME280Wrap());
				break;
			case ads1115:
				sensor_list->push_back(new ADS1115VoltmeterWrap(sensors[i].pin));
				break;
			case ads1115anemometer:
				sensor_list->push_back(new ADS1115AnemometerWrap(sensors[i].pin));
				break;
			case ads1115phototransistor:
				sensor_list->push_back(new ADS1115PhototransistorWrap(sensors[i].pin));
				break;
			case ina219:
				sensor_list->push_back(new INA219Wrap(sensors[i].pin));
				break;
			default:
				break;
			}
		}
	}

	// returns true when it's time for another reading
	// unused for now, but we may want to use this in the future for tighter timing
	bool timekeeper()
	{
		yield(); //TODO figure out when to actually use this yield thing
		unsigned long now_ms = millis();
		if (now_ms - last_poll > poll_interval)
		{
			last_poll = now_ms;
			return true;
		}
		else
		{
			return false;
		}
	}

	// command getter
	bool get_command()
	{
		if(Serial.available())
		{
		if(Serial.readString() == "poll")
		{
			return true;
		}
		}
		return false;
	}

	// reads data from sensors and stores as a csv string
	String read_from_sensors()
	{
		String datum;
		// record time
		if(is_first_poll)
		{ // starts time at 0
			is_first_poll = false;
			poll_start = millis();
			datum = String(0.0f,2);
		}
		else
		{ // keeps counting
			datum = String((millis() - poll_start)/1000.0f,2);
		}
    	Serial.println(datum);
		// get data from sensors
		LinkedList<SensorWrap*>::Node* current_sensor = sensor_list->head;
		while(current_sensor)
		{
			// record reading into string
			for (int i = 0; i < current_sensor->sensor->num_readings; i++)
			{
				// make sure a valid reading is taken
				float temp_f; // = current_sensor->sensor->getReading(i);
				do
				{
					temp_f = current_sensor->sensor->getReading(i);
				} while (isnan(temp_f));
				datum += "," + String(temp_f);
        Serial.println(temp_f, 5);
			}

			// go to next sensor
			current_sensor = sensor_list->get_next_node(current_sensor);
		}

		return datum;
	}

	// operates led
	void Led(bool isOn)
	{
		digitalWrite(led_pin, isOn);
	}
	// just blinks led forever. stops execution of any other code
	void error_blink()
	{
		//Serial.println("error");
		bool is_on = false;
		while (true)
		{
			is_on ^= true; // toggle led state
			Led(is_on);
			delay(100);
		}
	}
};

DataLogger data_logger(LED_PIN, POLL_INTERVAL);

void setup()
{
	Serial.begin(115200);
	delay(3000);
	data_logger.init_sensors(SENSORS, NUM_SENSORS);
}

void loop()
{
	data_logger.update();
}

