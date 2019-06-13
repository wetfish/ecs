# ecs
Environmental control systems

## sensorlogger.py
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
