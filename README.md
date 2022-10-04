# ecs
Environmental control systems

---

# sensorlogger.py
Used for logging temperature data to a raspberry pi's SD card. Requires DS18B20, DHT11, or DHT22 sensor.
### Super basic instructions

- Write raspberry pi OS to an SD card
- Run `sudo apt update`
- Then `sudo apt install python3-pip`
- Then `pip3 install -r requirements.txt`
- Then `python3 simplelogger.py 26 dht11`

#### Installation and usage
Requires python 3

Requires 1-wire interface to be enabled in raspi-config for ds18b20 sensor https://www.raspberrypi-spy.co.uk/2018/02/enable-1-wire-interface-raspberry-pi/

To install dependencies:

`pip install -r requirements.txt`
  
Usage:

`python3 sensorlogger.py [GPIO pin #] [Sensor type] {-f output filename}`

GPIO pin # and sensor type required. When -f is used, an output filename can be specified for the log file, defaults to [sensorname].log when -f is not used

GPIO pin # is the BCM pin #, not the board pin # https://raspberrypi.stackexchange.com/questions/12966/what-is-the-difference-between-board-and-bcm-for-gpio-pin-numbering

Sensor type is all lower case. dht11, dht22, or ds18b20

## quickcsvplotter.py
Plots the data collected by sensorlogger.py

Usage:

`python3 quickcsvplotter.py [csv filename 1] [csv filename 2] ... [csv filename n]`

Providing filenames in command line is optional. If no filenames are specified, all csvs in current directory will be plotted.

---

# sensorlogger.ino
Used to log data from a variety of sensors to an SD card, using a  NodeMCU ESP8266 devkit. Other devices can be used, but some cannot fit the full program onto the flash memory. Also prints data to serial (115200 BAUD) , so can be monitored while running as well. A continuously flashing LED indicates an error with either the SD card or one of the sensors.

### Supported sensors:
- DS18B20
- BME280
- Phototransistor (like PT334-6C)
- INA219
- Anemometer

### Log file:
The first line acts as column headers for the rest of the file. Each of the following lines consist of comma separated values for each of the measurements being taken. Each line is a seperate set of measurements.

### Error blink code:
- 2x blink: SD initialization error
- ~10x blink: SD init ok, but SD write error

## Flashing the NodeMCU ESP8266
To add support for NodeMCU board to Arduino IDE:

1. Go to File -> Preferences -> "Additional Boards Manager URLs:", and paste https://arduino.esp8266.com/stable/package_esp8266com_index.json into the box. Click OK.
2. Now click Tools -> Board: -> "Boards Manager...". In the seach bar, type "esp8266". Click on the esp8266 package, then click "Install".
3. Restart Arduino IDE. Click Tools -> Board: -> ESP8266 Boards -> NodeMCU 1.0 (ESP-12E Module)

To install required libraries:

- Click Tools -> Manage Libraries..., then enter [library name] into search bar. Click "Install". If you are asked if you also want to install missing dependencies, click "Install All".

Do the above step for the following libraries:
 - Dallas Temperature
 - Adafruit BME280 Library
 - Adafruit ADS1X15
 - Adafruit INA219

 Now you should be able to open the sensorlogger.ino sketch in the Arduino IDE and upload it to the NodeMCU ESP board.

 ## sensorlogger.ino parameters
 Near the top of sensorlogger.ino, there is a list of parameters than can be changed according to your own hardware setup.

 **LOG_FILENAME** -> This is a string indicating the name of the log file. It must fit the 8.3 short format, so it can have a maximum of 8 characters, a period, and then 3 more characters. Each time the program is restarted, a new file is created, so if your LOG_FILENAME = "log.txt", log.txt will be created on the SD card. On subsequent runs, an underscore and a numeral will be appended to the filename.

 **POLL_INTERVAL** -> This is how long to wait between sensor readings, in milliseconds. 

 **NO_SD** -> This is a boolean that allows the program to run without an SD card connected. Set to "true" to run program without SD card, "false" if you want to write data to SD.

 **SENSORS** -> This is an array that represents which sensors to take readings from. Each element is a struct of {(enum)[sensor type], (uint8_t)[pin number or hardware address]}. Valid sensor types are: ds18b20, voltmeter, bme280, ads1115, ads1115anemometer, and ina219. For the INA219, the second field will need to contain the hardware address. If you have not altered the INA219 module at all, the address is 0x40. For the BME280, the second field is irrelevant and any int works. For the devices connected through the ADS1115, the second field is the pin on that board.

 **DS18B20ADDRESSES** -> This is an array of the addresses of DS18B20 sensors being used and a corresponding label for each one. The addresses are device-specific, so will need to found by running ds18b20AddressFinder.ino with each of your sensors. The label will appear at the column header in the log file so we know which temperature readings come from which sensors.

**CS_PIN** -> This is the chip select (CS) pin for the SPI interface af the SD card. 

**LED_PIN** -> This is the pin for the indicator LED. By default it is one of the built-in LEDs on the NodeMCU, but can be an external one if desired.

# ds18b20AddressFinder.ino
Use this to find addresses of all DS18B20 sensors being used. Make sure to mark each sensor as well so they don't get mixed up. It only requires the DallasTemperature library, which should have already been installed.

## ds18b20AddressFinder.ino parameters

**ONEWIRE_PIN** -> This is the pin that the data wire of the DS18B20 is connected to.

## Getting your device addresses
1. Connect first sensor to NodeMCU with ds18b20AddressFinder.ino.
2. Connect NodeMCU to computer with Arduino IDE (or other serial monitor).
3. Open serial monitor and make sure it's at 115200 BAUD.
4. Every 5 seconds, the sensor's address will be printed. Copy that to sensorlogger.ino or just record it for later.
5. Repeat for each DS18B20 you intend to use. Remember to indicate somehow which sensor corresponds to which address.

---

# sensorserial.ino and get_data_from_mcu.py
Used to send data from a variety of sensors to a PC via USB, using a  NodeMCU ESP8266 devkit. A continuously flashing LED indicates an error with either the SD card or one of the sensors. This program is very similar to sensorlogger.ino, but it sends data to a PC running a python script rather than log to SD, and the python script controls the timiing of the sensor polls.

## Setting up NodeMCU ESP8266
Follow the same steps as for sensorlogger.ino. Then plug the NodeMCU via USB into a computer that will run the python script.

## get_data_from_mcu.py
To be run from terminal on a PC, Raspberry Pi, etc:

`python3 get_data_from_mcu.py [poll_interval]`

[poll_interval] is the time in seconds between sensor polls, and is an optional argument. Default poll interval is 3 seconds.

Whenever restarting script, NodeMCU should be restarted as well.
