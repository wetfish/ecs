#include <SPI.h>
#include <SD.h>
#include <DHT_U.h>
#include <DHT.h>
#include <DallasTemperature.h>
enum SensorModels
{dht11, dht22, ds18b20};
struct SensorInfo
{
	SensorModels model; // dht11, dht22, or ds18b20
	uint8_t pin; // data pin of that sensor
};

///////////////////////////////////////////
// Change filename, polling interval, and sensor types here  
// Sensor types so far: dht11, dht22, or ds18b20
const String LOG_FILENAME = "dht11log.txt";
const unsigned long POLL_INTERVAL = 3000; // in milliseconds
const SensorInfo SENSORS[] = 
{// {sensor type, pin #}
	{dht11, 5}
};
uint8_t num_sensors = sizeof(SENSORS) / sizeof(SENSORS[0]);
///////////////////////////////////////////
// Other Pin selection
// Pins 11, 12, and 13 (on arduino uno) are required for the SPI for the SD card reader, so don't use them.
const uint8_t CS_PIN = 4;// cs pin for sd card
const uint8_t LED_PIN = 8; // led pin for indicator. not required.

class Sensor_Wrap
{
	public:
	uint8_t num_readings; // how many readings to take from this sensor (a DHT22 has 2: temperature and humidity)
	String labels;
	virtual void init(){} // whatever needs to go in setup()
	virtual float getReading(uint8_t reading_num){return NAN;} // return invalid value
};

// DHT 
class DHT_Wrap : public Sensor_Wrap
{
	private:
	DHT dht;

	public:
	DHT_Wrap(uint8_t pin_num, uint8_t dht_type): dht(pin_num, dht_type)
	{
		num_readings = 2; // 1 temp and 1 humidity
		labels = "DHT-Temp(F), DHT-RH(%)";
	}

	void init()
	{
		Serial.println("DHT sensor online");
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

// DS18B20
class DS18B20_Wrap : public Sensor_Wrap
{
	private:
	OneWire onewire;
	DallasTemperature ds18b20;

	public:
	DS18B20_Wrap(uint8_t pin_num): onewire(pin_num) 
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
	LinkedList<Sensor_Wrap*>* sensor_list = new LinkedList<Sensor_Wrap*>();
	
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
		Serial.println("Poll Interval: " + String(poll_interval));

		// set up led
		pinMode(led_pin, OUTPUT);
		
		// check if sd card is not set up
		if (!SD.begin(cs_pin))
		{
			// error
			Serial.println("Error, cannot find SD card on pin " + String(cs_pin));
			error_blink();
		}
		// get the right filename
		log_fn = set_next_filename();
	}

	void init_sensors(const SensorInfo sensors[], uint8_t& num_sensors)
	{
		list_sensors(sensors, num_sensors);
		LinkedList<Sensor_Wrap*>::Node* current_sensor = sensor_list->head;
		while(current_sensor)
		{
			current_sensor->sensor->init();
			column_labels += ", " + current_sensor->sensor->labels;
			current_sensor = sensor_list->get_next_node(current_sensor);
		}
		// show what each column means
		Serial.println("\n" + column_labels);
		Serial.println("-------------------------------");
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
				Serial.println("Error opening from file \"" + log_fn + "\"");
				Serial.println("Filename must be in short 8.3 format."); // 8 characters + . + 3 characters
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
				sensor_list->push_back(new DHT_Wrap(sensors[i].pin, DHT11));
				break;
			case dht22:
				sensor_list->push_back(new DHT_Wrap(sensors[i].pin, DHT22));
				break;
			case ds18b20:
				sensor_list->push_back(new DS18B20_Wrap(sensors[i].pin));
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
		LinkedList<Sensor_Wrap*>::Node* current_sensor = sensor_list->head;
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



