#include <SPI.h>
#include <SD.h> // http://www.arduino.cc/en/Reference/SD
//#include <DHT_U.h> // https://github.com/adafruit/DHT-sensor-library
//#include <DHT.h>  // https://github.com/adafruit/DHT-sensor-library
#include <DallasTemperature.h>  // https://www.milesburton.com/Dallas_Temperature_Control_Library (for DS18B20)
#include <Adafruit_BME280.h> // https://github.com/adafruit/Adafruit_BME280_Library 
#include <Adafruit_ADS1X15.h> // https://github.com/adafruit/Adafruit_ADS1X15 (ADC board)
#include <Adafruit_INA219.h> // https://github.com/adafruit/Adafruit_INA219 (Volt/Ammeter)

// These are the valid sensor models than can be used.
enum SensorModels
{/*dht11, dht22,*/ ds18b20, voltmeter, bme280, ads1115, ads1115anemometer, ina219};
struct SensorInfo
{
	SensorModels model;
	uint8_t pin; // data pin of that sensor - for I2C devices, this usually is arbitrary
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
// Change filename, polling interval, SD logging, and sensor types here
const String LOG_FILENAME = "log.txt"; // first line is column labels, so throw that away when using the file for computations
const unsigned long POLL_INTERVAL = 3000; // in milliseconds
const bool NO_SD = false; // disables sd card logging
const SensorInfo SENSORS[] = 
{// {sensor model, pin # (or I2C address for INA219)}
	{ina219, 0x40}, {ina219, 0x41}//{ads1115, Ads1115Pins::A0}//, {ds18b20, NodeMcuPins::SD3}
};
uint8_t num_sensors = sizeof(SENSORS) / sizeof(SENSORS[0]);

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

/*
// DHT sensors
class DhtWrap : public SensorWrap
{
	private:
	DHT dht;

	public:
	DhtWrap(uint8_t pin_num, uint8_t dht_type): dht(pin_num, dht_type)
	{
		num_readings = 2; // 1 temp and 1 humidity
		labels = "DHT-Temp(F), DHT-RH(%)";
	}

	void init()
	{
		Serial.println("DHT init");
		dht.begin();
	}

	float getReading(uint8_t reading_num)
	{
		switch (reading_num)
		{
			case 0:
				return dht.readTemperature(true); // true for F, false for C
			case 1:
				return dht.readHumidity();
			default:
				return NAN;
		}
	}
};
*/


/* ----------------
 * DS18B20 sensor: 
 * Reads temperature. Many identical sensors can be wired to the same pin.
 * num_readings does not need to be set manually. It's set in init() automatically.
 * pin_num = OneWire data pin number
 */
class DS18B20Wrap : public SensorWrap
{
	private:
	OneWire onewire;
	DallasTemperature ds18b20;

	public:
	DS18B20Wrap(uint8_t pin_num): onewire(pin_num) 
	{
		num_readings = 1;
		labels = "DS18-Temp(F)";
	}

	void init()
	{
		ds18b20.setOneWire(&onewire);
		ds18b20.begin();
		num_readings = ds18b20.getDeviceCount();
	}

	float getReading(uint8_t reading_num)
	{
		if(reading_num == 0)
		{  // only request temps once for all readings
			ds18b20.requestTemperatures();
		}
		return ds18b20.getTempFByIndex(reading_num); // more sensors can be on the same onewire
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
		labels = "Voltage";
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
 * //TODO: make this class more general so it can handle 1-4 measurements
 */
class ADS1115VoltmeterWrap : public SensorWrap
{
	
	// Set Gain here. It determines voltage input range.
	// Things might go badly if you exceed range. Maybe pick a larger range until
	//  you're certain of the full input range.
	//    GAIN           Range   |     GAIN         Range
	// GAIN_TWOTHIRDS: +-6.144V  |  GAIN_FOUR:    +-1.024V
	// GAIN_ONE:       +-4.096V  |  GAIN_EIGHT:   +-0.512V
	// GAIN_TWO:       +-2.048V  |  GAIN_SIXTEEN: +-0.256V
	const adsGain_t GAIN = GAIN_TWOTHIRDS;
	protected:
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
		labels = "Voltage";
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
	// GAIN_ONE corresponds to range of +-4.096V
	// GAIN_TWO corresponds to range of +-2.048V
	// Anemometer should return max V of 2V. If true, consider changing 
	//  to GAIN_TWO for higher precision.
	const adsGain_t GAIN = GAIN_ONE;
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

/* ----------------------
 * INA219 voltmeter & ammeter
 * -uses I2C: D1(SCL) and D2(SDA) on NodeMCU,  A5(SCL) and A4(SDA) on Arduino Uno and Pro Mini
 * I2C default address = 0x40, secondary is 0x41 (short A0 to VCC)
 * Vin+ and Vin- must be wired in series with current to be measured. Vin+ is high side, Vin- is low.
 * Can measure up to ~3.2A without breaking. Limiting component is shunt resistor. Sensor can 
 *  measure up to ~10-15A if resistor is swapped for one of even lower resistance.
 * //TODO: Possibly also take power measurements? Or just calculate them?
 */
class INA219Wrap : public SensorWrap
{
	private:
	Adafruit_INA219 ina;

	public:
	INA219Wrap(uint8_t i2cAddr) : ina(i2cAddr)
	{
		char addr_hex[2];
		sprintf(addr_hex, "%02X", i2cAddr);
		num_readings = 2; // Amps and Volts
		labels = "Current(mA)[0x" + String(addr_hex) + "], Voltage(V)[0x" + String(addr_hex) + "]";
	}

	void init()
	{
		Serial.println("INA init");
		ina.begin();
	}

	float getReading(uint8_t reading_num)
	{
		switch (reading_num)
		{
			case 0:
				return ina.getCurrent_mA();
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
	uint8_t cs_pin;
	uint8_t led_pin;
	unsigned long poll_interval;
	unsigned long last_poll;
	LinkedList<SensorWrap*>* sensor_list = new LinkedList<SensorWrap*>();
	bool did_err_msg = false;
	
	public:
	/* 
	 * logFn- filename should be short so as to conform to 8.3 short format (8 characters + '.' + 3 characters)
	 * pollInterval- time between sensor polls in milliseconds	 *
	 */
	DataLogger(String logFn, uint8_t CSpinNo, uint8_t LEDpinNo, unsigned long pollInterval) : 
	log_fn(logFn) 
	{
		cs_pin = CSpinNo;
		led_pin = LEDpinNo;
		poll_interval = pollInterval;
		last_poll = 0;
		column_labels = "Time(s)";
	}
	
	// initializers. these are run in setup()
	void init_sd()
	{
		if(NO_SD)
		{
			return;
		}
		Serial.println("CS_pin: " + String(cs_pin));
		Serial.println("led_pin: " + String(led_pin));
		Serial.println("Poll Intvl: " + String(poll_interval));

		// set up led
		pinMode(led_pin, OUTPUT);
		Led(false);

		// check if sd card is not set up
		if (!SD.begin(cs_pin))
		{
			// error
			Serial.println("SD err " + String(cs_pin));
			error_blink();
		}
		// get the right filename
		log_fn = set_next_filename();
	}

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
		// show what each column means
		Serial.println("\n" + column_labels);
		if(!NO_SD)
		{	// when parsing the CSV, either throw away all lines starting with #, or just the first line
			write_file("# " + column_labels);
		}
		//Serial.println("-------------------------------");
	}

	// takes readings and writes to file
	void update()
	{
		if(timekeeper())
		{
			String sensor_data = read_from_sensors();
			Serial.println(sensor_data);
			// if not using sd stop here
			if(NO_SD)
			{
				return;
			}
			write_file(sensor_data);
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
			//case dht11:
			//	sensor_list->push_back(new DhtWrap(sensors[i].pin, DHT11));
			//	break;
			//case dht22:
			//	sensor_list->push_back(new DhtWrap(sensors[i].pin, DHT22));
			//	break;
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
			case ina219:
				sensor_list->push_back(new INA219Wrap(sensors[i].pin));
				break;
			default:
				break;
			}
		}
	}

	// increments filename if one already exists with given name.
	String set_next_filename()
	{
		String temp_fn = log_fn;
		String base_fn;
		String extension;
		unsigned long i = 1;
		if (SD.exists(temp_fn))
		{
			size_t dot_loc= temp_fn.lastIndexOf(".");
			base_fn = temp_fn.substring(0, dot_loc);
			extension = temp_fn.substring(dot_loc);

			while (SD.exists(temp_fn))
			{
				String i_string = "_" + String(i);
				if (base_fn.length() + i_string.length() > 8)
				{
					base_fn = base_fn.substring(0, 8 - i_string.length());
				}
				temp_fn = base_fn + i_string + extension;
				i++;
			}
		}
		Serial.println("Filename=\"" + temp_fn + "\"");
		return temp_fn;
	}

	// returns true when it's time for another reading
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

	// reads data from sensors and stores as a csv string
	// uses last_poll as time value
	String read_from_sensors()
	{
		// record time
		String datum = String(last_poll/1000);
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
			}

			// go to next sensor
			current_sensor = sensor_list->get_next_node(current_sensor);
		}

		return datum;
	}

	// writes string to file. Opens file, writes, closes file.
	void write_file(String to_write)
	{
		File log_file = SD.open(log_fn, FILE_WRITE);
		if(!log_file) // if file can't be opened, show error
		{
			Serial.println("Error opening file \"" + log_fn + "\"");
			Serial.println("Fname must be in 8.3 format."); // 8 characters + . + 3 characters
			error_blink();
		}
		else// otherwise, everything is fine
		{
			log_file.println(to_write);
			log_file.close();
		}
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

DataLogger data_logger(LOG_FILENAME, CS_PIN, LED_PIN, POLL_INTERVAL);

void setup()
{
	Serial.begin(115200);

	data_logger.init_sd();
	data_logger.init_sensors(SENSORS, num_sensors);

	yield(); //TODO prob don't need this yield
}

void loop()
{
	data_logger.update();
}
