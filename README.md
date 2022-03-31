# ecs
Environmental control systems

## sensorlogger.py
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

## simplelogger.ino
#### Installation and usage
- DHT sensor's data line is connected to pin 5.
- SD card reader is connected as follows:
	+ CS to pin 4
	+ MOSI to pin 11
	+ CLK to pin 13
	+ MISO to pin 12
- Optionally, an indicator LED is connected to pin 8

Pin numbers, output filename, and DHT sensor type can be changed at the top of sensorlogger.ino. Filename must use short 8.3 format (8 character name and 3 character extension) Every time the arduino is booted it will create a csv file to record time, temperature, and relative humidity. Since it has no real time clock, the time is recorded in seconds since boot. 

The indicator LED will light while readings are being taken. If there are any errors with the SD card reader, the device will flash the indicator LED, and will need to be reset.

## quickcsvplotter.py
Plots the data collected by sensorlogger.py

Usage:

`python3 quickcsvplotter.py [csv filename 1] [csv filename 2] ... [csv filename n]`

Providing filenames in command line is optional. If no filenames are specified, all csvs in current directory will be plotted.
