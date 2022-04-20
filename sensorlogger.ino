#include <SPI.h>
#include <SD.h> // http://www.arduino.cc/en/Reference/SD
#include <DHT_U.h> // https://github.com/adafruit/DHT-sensor-library
#include <DHT.h>  // https://github.com/adafruit/DHT-sensor-library
#include <DallasTemperature.h>  // https://www.milesburton.com/Dallas_Temperature_Control_Library (for DS18B20)
#include <Adafruit_BME280.h> // https://github.com/adafruit/Adafruit_BME280_Library 
#include <Adafruit_ADS1X15.h> // https://github.com/adafruit/Adafruit_ADS1X15 (ADC board)
//#include <Adafruit_INA219.h> https://github.com/adafruit/Adafruit_INA219 (Volt/Ammeter)

enum SensorModels
{dht11, dht22, ds18b20, voltmeter, bme280, ads1015, ads1015anemometer, ina219};
struct SensorInfo
{
	SensorModels model;
	uint8_t pin; // data pin of that sensor
};

///////////////////////////////////////////
// Change filename, polling interval, and sensor types here  
// Sensor types so far: dht11, dht22, or ds18b20
const String LOG_FILENAME = "log.txt";
const unsigned long POLL_INTERVAL = 3000; // in milliseconds
const SensorInfo SENSORS[] = 
{// {sensor type, pin #}
	{dht11, 5}
};
uint8_t num_sensors = sizeof(SENSORS) / sizeof(SENSORS[0]);
///////////////////////////////////////////
// Other Pin selection
// Pins 11, 12, and 13 (on arduino uno) are required for the SPI for the SD card reader, so don't use them.
// Pins 18 and 19 (or is it A4 & A5?) are used for I2C communication, so don't use them either.
const uint8_t CS_PIN = 4;// cs pin for sd card
const uint8_t LED_PIN = 8; // led pin for indicator. not required.

////////////////////////////////////////////
// When adding new sensors, just make a wrapper that has the following variables and methods:
class SensorWrap
{
	public:
	uint8_t num_readings = 1; // how many readings to take from this sensor (a DHT22 has 2: temperature and humidity)
	String labels; // label(s) for the data gathered from sensor
	virtual void init(){} // whatever needs to go into setup()
	virtual float getReading(uint8_t reading_num){return NAN;} // return measured value (NAN if no reading taken)
};


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


// DS18B20 sensor
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
	}

	float getReading([[maybe_unused]] uint8_t reading_num)
	{
		ds18b20.requestTemperatures();
		return ds18b20.getTempFByIndex(0);
	}
};


// Voltmeter - scale of 1 maps analog reading to 0 - 5V. if using a voltage divider, apply approriate scale.
class VoltmeterWrap : public SensorWrap
{	
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
		int reading = analogRead(analog_pin); // returns value from 0-1024
		float V = scale * (reading * 5.0f) / 1024.0f; 
		return V;
	}
};


// BME280 sensor using I2C : A4 & A5 on arduino uno and arduino pro mini
class BME280Wrap : public SensorWrap
{
	private:
	// set which values to measure - reorder the switch statement if there are any true after a false
	const bool IS_TAKING_P = true;
	const bool IS_TAKING_RH = true;
	const bool IS_TAKING_TEMP = false;
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
		bme.begin();
	}

	float getReading(uint8_t reading_num)
	{
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

// ADS1015 Analog to Digital Converter - using I2C
// Gets one reading at a time. Process that reading based on what you're measuring.
class ADS1015VoltmeterWrap : public SensorWrap
{
	
	// Set Gain here. It determines voltage input range.
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

	Adafruit_ADS1015 ads;
	uint8_t pin;
	public:
	ADS1015VoltmeterWrap(uint8_t ADCpin): pin(ADCpin)
	{
		ads.setGain(GAIN);
		num_readings = 1;
	}
	virtual void init()
	{
		Serial.println("ADC init");
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

class ADS1015AnemometerWrap : public ADS1015VoltmeterWrap
{
	public:
	using ADS1015VoltmeterWrap::ADS1015VoltmeterWrap;
	void init()
	{
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

/*
// INA219 sensor using I2C : A4 & A5 on arduino uno and arduino pro mini
class INA219Wrap : public SensorWrap
{
	private:
	Adafruit_INA219 ina;

	public:
	INA219Wrap()
	{
		num_readings = 2; // Amps and Volts
		labels = "Current(mA), Voltage(V)";
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
*/

// List for different sensors
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

// for creating list of attached sensors and writing data to SD, at given frequency
class Data_Logger
{
	String log_fn;
	String column_labels;
	uint8_t cs_pin;
	uint8_t led_pin;
	unsigned long poll_interval;
	unsigned long last_poll;
	LinkedList<SensorWrap*>* sensor_list = new LinkedList<SensorWrap*>();
	
	public:
	Data_Logger(String logFn, uint8_t CSpinNo, uint8_t LEDpinNo, unsigned long pollInt) : 
	log_fn(logFn) 
	{
		cs_pin = CSpinNo;
		led_pin = LEDpinNo;
		poll_interval = pollInt;
		last_poll = 0;
		column_labels = "Time(s)";
	}
	
	// initializers. these go in setup()
	void init_sd()
	{
		Serial.println("CS_pin: " + String(cs_pin));
		Serial.println("led_pin: " + String(led_pin));
		Serial.println("Poll Intvl: " + String(poll_interval));

		// set up led
		pinMode(led_pin, OUTPUT);
		
		// check if sd card is not set up
		if (!SD.begin(cs_pin))
		{
			// error
			Serial.println("SD err" + String(cs_pin));
			error_blink();
		}
		// get the right filename
		log_fn = set_next_filename();
	}

	void init_sensors(const SensorInfo sensors[], uint8_t& num_sensors)
	{
		list_sensors(sensors, num_sensors);
		LinkedList<SensorWrap*>::Node* current_sensor = sensor_list->head;
		while(current_sensor)
		{
			current_sensor->sensor->init();
			column_labels += ", " + current_sensor->sensor->labels;
			current_sensor = sensor_list->get_next_node(current_sensor);
		}
		// show what each column means
		Serial.println("\n" + column_labels);
		//Serial.println("-------------------------------");
	}

	// keeps track of poll interval, takes readings, and writes to file
	void write_to_log()
	{
		if(timekeeper())
		{
			// open file
			File log_file = SD.open(log_fn, FILE_WRITE);
			if(!log_file) // if file can't be opened, show error
			{
				Serial.println("Error opening file \"" + log_fn + "\"");
				Serial.println("Fname must be in 8.3 format."); // 8 characters + . + 3 characters
				//error_blink();
			}
			//else// otherwise, everything is fine
			{
				String temp_s = read_from_sensors();
				log_file.println(temp_s);
				Serial.println(temp_s);
				log_file.close();
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
			case dht11:
				sensor_list->push_back(new DhtWrap(sensors[i].pin, DHT11));
				break;
			case dht22:
				sensor_list->push_back(new DhtWrap(sensors[i].pin, DHT22));
				break;
			case ds18b20:
				sensor_list->push_back(new DS18B20Wrap(sensors[i].pin));
				break;
			case voltmeter:
				sensor_list->push_back(new VoltmeterWrap(sensors[i].pin));
				break;
			case bme280:
				sensor_list->push_back(new BME280Wrap());
				break;
			case ads1015:
				sensor_list->push_back(new ADS1015VoltmeterWrap(sensors[i].pin));
				break;
			case ads1015anemometer:
				sensor_list->push_back(new ADS1015AnemometerWrap(sensors[i].pin));
				break;
			case ina219:
			//	sensor_list->push_back(new INA219Wrap());
				break;
			default:
				break;
			}
		}
	}

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
				float temp_f = current_sensor->sensor->getReading(i);
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

Data_Logger data_logger(LOG_FILENAME, CS_PIN, LED_PIN, POLL_INTERVAL);

void setup()
{
	Serial.begin(9600);

	// initilize sd and sensors
	data_logger.init_sd();
	data_logger.init_sensors(SENSORS, num_sensors);

}

void loop()
{
	data_logger.write_to_log();
}
