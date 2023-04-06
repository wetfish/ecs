#include <SPI.h>
#include <SD.h> // http://www.arduino.cc/en/Reference/SD

/* 
https://www.milesburton.com/Dallas_Temperature_Control_Library (for DS18B20)
https://github.com/adafruit/Adafruit_BME280_Library 
https://github.com/adafruit/Adafruit_ADS1X15 (ADC board)
https://github.com/adafruit/Adafruit_INA219 (Volt/Ammeter)
*/

#include "sensorwrap.h"
#include "ds18b20wrap.h"
#include "voltmeterwrap.h"
#include "bme280wrap.h"
#include "ads1115voltmeterwrap.h"
#include "ads1115anemometerwrap.h"
#include "ads1115phototransistorwrap.h"
#include "ina219wrap.h"

#include "oleddisplay.h"

// These are the valid sensor models than can be used.
enum SensorModels
{/*dht11, dht22,*/ ds18b20, voltmeter, bme280, ads1115, ads1115anemometer, ads1115phototransistor, ina219};
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
	{ina219, 0x40}, 
	{ina219, 0x41}, 
	//{ads1115, Ads1115Pins::A0}, 
	{bme280, 0},
	{ds18b20, NodeMcuPins::SD3},
	{ads1115phototransistor, Ads1115Pins::A0},
	{ads1115anemometer, Ads1115Pins::A1},
	//{voltmeter, A0}
};

// --- DS18B20 Addresses ---
// List sensor addresses here. Get them individually with ds18b20AddressFinder.ino or similar
// Labels should be where the sensor is located or something. Or some identifying factor.
const DS18B20Addresses DS18B20_ADDRESSES[] = {
		//{{0x28, 0x63, 0x1B, 0x49, 0xF6, 0xAE, 0x3C, 0x88}, "MAIN"}, // measuring electronics enclosure temp,has white heatshrink
		{{0x28, 0xDC, 0xA0, 0x49, 0xF6, 0xEB, 0x3C, 0xF0}, "Batt"}, // measuring battery enclosure temp, has white heatshrink
		{{0x28, 0x6A, 0x64, 0x49, 0xF6, 0xBE, 0x3C, 0xF0}, "MCU"} // has red heatshrink
		};

uint8_t NUM_SENSORS = sizeof(SENSORS) / sizeof(SENSORS[0]);
uint8_t NUM_DS18B20S = sizeof(DS18B20_ADDRESSES) / sizeof(DS18B20_ADDRESSES[0]);

//----------------------------------------------------------
// Other Pin selection
// Pins D5 (SCK), D6(MISO), D7(MOSI), and D8(CS) on NodeMCU are required for the SPI for the SD card reader, so don't use them.
// On NodeMCU, pin D1(SCL) and D2(SDA) for I2C communication
const uint8_t CS_PIN = NodeMcuPins::D8;// cs pin for sd card
const uint8_t LED_PIN = LED_BUILTIN_AUX; // led pin for indicator. not required.

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

	float takeReading(uint8_t reading_num)
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

	// add node to end of list and return pointer to that node, in case it's needed
	Node* push_back(T newData)
	{
		Node* new_node = get_new_node(newData);
		if (head == nullptr) // if this ia the start of the list
		{
			head = new_node;
		}
		else                // if this is adding to an existing list
		{
			Node* curr_node = head;
			while (curr_node->next)
			{
				curr_node = curr_node->next;
			}
			curr_node->next = new_node;
			list_size++;
		}
		return new_node;
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

	uint8_t total_num_readings; // total number of readings per update
	float last_sensor_readings[25]; // this should be large enough for all of our readings.

	OledDisplay display;
	
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
		total_num_readings = 1; // we will always at least be recording time
	}
	
	// initializers. these are run in setup()
	void init_sd()
	{
		if(NO_SD)
		{
			Serial.println("Not logging to SD.");
			return;
		}
		Serial.println("CS_pin: " + String(cs_pin));
		Serial.println("led_pin: " + String(led_pin));
		Serial.println("Poll Intvl: " + String(poll_interval));

		// set up led
		pinMode(led_pin, OUTPUT);
		Led(true);

		// check if sd card is not set up
		while (!SD.begin(cs_pin))
		{
			// error
			Serial.println("SD err " + String(cs_pin));
			//error_blink();
			// blink 2x quick, then try again
			Led(true);
			delay(100);
			Led(false);
			delay(100);
			Led(true);
			delay(100);
			Led(false);
			delay(700);

		}
		Led(true);
		// get the right filename
		log_fn = set_next_filename();
	}

	void init_sensors(const SensorInfo sensors[], uint8_t& num_sensors)
	{
		display.init();
		list_sensors(sensors, num_sensors);
		// iterate through list of sensors to get the column labels from all of them
		LinkedList<SensorWrap*>::Node* current_sensor = sensor_list->head;
		while(current_sensor)
		{
			current_sensor->sensor->init();
			column_labels += ", " + current_sensor->sensor->labels;
			total_num_readings += current_sensor->sensor->num_readings;
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
			oled_display();
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
			{
				LinkedList<SensorWrap*>::Node* this_node;
				this_node = sensor_list->push_back(new DS18B20Wrap(sensors[i].pin));
				LinkedList<DS18B20Wrap*>::Node* ds_node = (LinkedList<DS18B20Wrap*>::Node*)this_node;
				ds_node->sensor->set_addresses(DS18B20_ADDRESSES, NUM_DS18B20S);
			}
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
		last_sensor_readings[0] = last_poll/1000;
		// get data from sensors
		LinkedList<SensorWrap*>::Node* current_sensor = sensor_list->head;
		// keep track of readings
		uint8_t reading_index = 1;
		while(current_sensor)
		{
			// record reading into string
			for (int i = 0; i < current_sensor->sensor->num_readings; i++)
			{
				// make sure a valid reading is taken
				float temp_f; // = current_sensor->sensor->getReading(i);
				uint8_t num_tries = 0; // count how many tries to read, so can timeout
				do
				{
					temp_f = current_sensor->sensor->getReading(i);
					if (num_tries > 10) // 10 tries before timeout sounds good?
					{
						break;
					}
					num_tries++;
				} while (isnan(temp_f));
				datum += "," + String(temp_f);
				last_sensor_readings[reading_index] = temp_f;
				reading_index++;
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
		uint8_t retry_num = 0;
		while(!log_file) // if file can't be opened, show error
		{
			Serial.println("Error opening file \"" + log_fn + "\"");
			Serial.println("Fname must be in 8.3 format."); // 8 characters + . + 3 characters
			retry_num++;
			delay(100);
			if (retry_num > 30) // resets device after a while
			{
				error_blink();
			}
		}

		// otherwise, everything is fine
		log_file.println(to_write);
		log_file.close();
	}

	// formats data for oled display
	void oled_display()
	{
		// which sensors do we want to display on the oled (10 max i think  for the 128x32)
		static const String ID_LABELS[] = // These must == exact string label in the corresponding sensor's class
		{
			"BME280 Temp(F)", "BME280 Humidity(%)", "DS18 Temp(F)[Batt]", "Voltage[0x40]", "Power(mW)[0x40]", "Wind Speed"
		};
		static const String DISPLAY_LABELS[] = // This will be displayed on the oled before the measurement. match these to ID_LABELS.
		{
			"Air:", "Air:", "Batt:", "Batt:", "Batt:", "Wnd:"
		};
		static const String DISPLAY_UNITS[] = // This will be displayed on the oled after the measurement. match these to ID_LABELS
		{
			"F", "%", "F", "V", "mW", "mph"
		};
		uint8_t NUM_DISPLAYED_VALUES = sizeof(ID_LABELS) / sizeof(ID_LABELS[0]);
		float display_values[7]; // 4 lines on the display, 2 values can fit per line, but time will take one slot
			
		// keep track of readings
		for(uint8_t label_index = 0; label_index < NUM_DISPLAYED_VALUES; label_index++)
		{
			// iterate through sensor list and get last readings
			LinkedList<SensorWrap*>::Node* current_sensor = sensor_list->head;
			
			// flag to determine if we found the sensor we want
			bool found_sensor = false;
			while(current_sensor)
			{
				// record reading into string
				String current_full_label = current_sensor->sensor->labels;
				
				for (uint8_t i = 0; i < current_sensor->sensor->num_readings; i++)
				{	
					//print the labels for this sensor and the label we are looking for
					//Serial.println("current_full_label: " + current_full_label + ".\nlooking for:        " + ID_LABELS[label_index]+".");
					// if the current reading is the one we want
					if(current_full_label == ID_LABELS[label_index] || current_full_label.startsWith(ID_LABELS[label_index]))
					{
						display_values[label_index] = current_sensor->sensor->last_reading[i];
						found_sensor = true;
						break;
					}
					// if there are more sensor readings left on this sensor, cut the beginning off the label
					if(i+1 < current_sensor->sensor->num_readings) 
					{
						// snip the first label off the string, along with the comma and space
						current_full_label = current_full_label.substring(current_full_label.indexOf(',') + 2);
					}
				}
				// if we haven't found the sensor yet
				if(!found_sensor)
				{
					display_values[label_index] = -196.6; // error code
				}

				// go to next sensor
				current_sensor = sensor_list->get_next_node(current_sensor);
			}
		}

		// make strings for display - the array size should = display_values[] + 1
		String display_value_strings[8];
		// make sure each value is less than 4 characters long. if not, round it
		for(uint8_t i = 0; i < NUM_DISPLAYED_VALUES; i++)
		{		
			// if our sensor is errored, display dashes
			if(abs(display_values[i] + 196.6) < 0.01)
			{
				display_value_strings[i] = "--";
				continue;
			}

			else if (display_values[i] < 10)
			{
				display_values[i] = round(display_values[i] * 100) / 100;
			}
			else if (display_values[i] < 100)
			{
				display_values[i] = round(display_values[i] * 10) / 10;
			}
			else
			{
				display_values[i] = round(display_values[i]);
			}
			// convert to string and remove trailing zeros
			display_value_strings[i] = removeTrailingZeros(String(display_values[i]));
			// check if display will be greater than 10 char
			if((display_value_strings[i] + DISPLAY_UNITS[i] + DISPLAY_LABELS[i]).length() > 10)
			{
				// if so, round it some more
				// if we need one less character:
				if ((display_value_strings[i] + DISPLAY_UNITS[i] + DISPLAY_LABELS[i]).length() - 10 == 1)
				{
					if (display_values[i] < 10)
					{
						display_values[i] = round(display_values[i] * 10) / 10;
					}
					else
					{
						display_values[i] = round(display_values[i]);
					}
				}
				// if we need two less characters:
				else
				{
					display_values[i] = round(display_values[i]);
				}
				// convert to string and remove trailing zeros
				display_value_strings[i] = removeTrailingZeros(String(display_values[i]));
			}
		}

		String display_string[4];
		// 4 lines of display
		for (uint8_t i = 0; i < 4; i++)
		{
			if(i < NUM_DISPLAYED_VALUES)
			{
				display_string[i] = DISPLAY_LABELS[i] + display_value_strings[i] + DISPLAY_UNITS[i];
				// add spaces to make sure the second column is aligned (column 1 is 10 chars long)
				while(display_string[i].length() < 10)
				{
					display_string[i] += " ";
				}
				if(i + 4 < NUM_DISPLAYED_VALUES)
				{	// second column stuff, if it exists
					display_string[i] += " " + DISPLAY_LABELS[i+4] + display_value_strings[i+4] + DISPLAY_UNITS[i+4];
				}
			}
		}
		if (NUM_DISPLAYED_VALUES < 4)
		{
			display_string[3] = "          ";
		}		
		display_string[3] += " t:" + String(float(last_poll) / 3600000) + "hrs";
		display.display(display_string);
	}

	// converts float to string and removes trailing zeros
	String floatToString(float value) {
	char result[12]; // increase size if needed
		dtostrf(value, 0, 2, result);  // convert float to char array with 2 decimal places (change this value as needed)
		String stringResult = String(result);

		// Removes any trailing zeros from decimal point
		int lastIndex = stringResult.length() - 1;
		while (stringResult.charAt(lastIndex) == '0') {
			lastIndex--;
		}
		if (stringResult.charAt(lastIndex) == '.') {
			lastIndex--;
		}
		stringResult.remove(lastIndex + 1, stringResult.length());

		return stringResult;
	}

	// remove trailing zeros from a string
	String removeTrailingZeros(String str)
	{
		// make sure there is a decimal point
		if (str.indexOf('.') == -1)
		{
			return str;
		}
		// remove trailing zeros
		int lastIndex = str.length() - 1; // index of last char
		while (str.charAt(lastIndex) == '0') // if there is a zero, mark it for removal
		{
			lastIndex--;
		}
		if (str.charAt(lastIndex) == '.') // if there is a decimal point, mark it for removal
		{
			lastIndex--;
		}
		str.remove(lastIndex + 1, str.length()); // remove all chars after the last non-zero char
		return str;
	}
	
	// operates led
	void Led(bool isOn)
	{
		digitalWrite(led_pin, !isOn);
	}
	// just blinks led forever. stops execution of any other code, then resets
	void error_blink()
	{
		//Serial.println("error");
		bool is_on = false;
		uint8_t blink_times = 0;
		while (true)
		{
			is_on ^= true; // toggle led state
			Led(is_on);
			delay(100);
			blink_times++;
			while (blink_times > 30){} // after few sec, get stuck - then reset through wdt
		}
	}
};

DataLogger data_logger(LOG_FILENAME, CS_PIN, LED_PIN, POLL_INTERVAL);

void setup()
{
	Serial.begin(115200);
	delay(3000);
	data_logger.init_sd();
	data_logger.init_sensors(SENSORS, NUM_SENSORS);
}

void loop()
{
	data_logger.update();
}
