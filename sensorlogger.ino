/*
 Name:		sensorlogger.ino
 Created:	3/26/2022 12:57:34 AM
 Author:	Blood Bag
*/

#include <SPI.h>
#include <SD.h>
#include <DHT_U.h>
#include <DHT.h>

#define CS_PIN 4 // cs pin for sd card
const String LOG_FILENAME = "dht11log.txt";

#define DHT_PIN 5
#define DHT_TYPE DHT11

#define LED_PIN 8


class DHTLogger
{
private:
	const unsigned long poll_interval = 3000; // in ms

	uint8_t led_pin;
	uint8_t cs_pin;
	String log_fn;
	unsigned long last_ms;
	DHT dht;

	// gets data and writes to log file. returns false if it couldn't get a good reading
	bool log_data()
	{
		// get temp and humidity
		float temp_f, rh;
		Led(true); // visual indicator of how long it takes to poll
		temp_f = dht.readTemperature(true); // true for fahrenheit
		rh = dht.readHumidity();
		Led(false);
		// check we got them
		if (isnan(temp_f) || isnan(rh))
		{
			return false;
		}

		// Write to log
		File log_file = SD.open(log_fn, FILE_WRITE);
		if (!log_file) // if the file's not working right
		{
			// error
			Serial.println("Error opening file \"" + log_fn + "\"");
			Serial.println("Filename must be short 8.3 form.");
			error_blink();
		}
		else // everything's ok
		{
			String temp = String(millis()/1000) + "," + String(temp_f) + "," + String(rh);
			log_file.println(temp);
			Serial.println(temp);
			log_file.close();
		}
		return true;
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

public:
	DHTLogger(uint8_t dhtPin, uint8_t dhtType, uint8_t csPin, uint8_t ledPin, String logFn): dht(dhtPin, dhtType)
	{
		led_pin = ledPin;
		cs_pin = csPin;
		log_fn = logFn;
		last_ms = 0;
	}
	
	// initialize led, sd reader, and dht sensor stuff
	void init()
	{
		// set up led
		pinMode(led_pin, OUTPUT);

		// check if sd card is not set up
		if (!SD.begin(cs_pin))
		{
			// error
			error_blink();
		}
		// get the right filename
		log_fn = set_next_filename();

		// start sensor
		dht.begin();
	}

	// run data 
	void run()
	{
		unsigned long now_ms = millis();
		if (now_ms - last_ms > poll_interval)
		{
			if (log_data())
			{
				last_ms = now_ms;
			}
		}
	}
};

DHTLogger dht_log(DHT_PIN, DHT_TYPE, CS_PIN, LED_PIN, LOG_FILENAME);

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(9600);
	dht_log.init();
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	dht_log.run();
}
