#!/usr/bin/python
import Adafruit_DHT
from time import sleep, strftime, time
import csv

# Type of sensor, either DHT11 or DHT22
sensor = Adafruit_DHT.DHT11

#GPIO pin number
pin = 18

# open csv log
with open("dht11.log", "a") as log:

    while True:
        # Read from the sensor
        hum, temp = Adafruit_DHT.read_retry(sensor, pin)
        if hum is not None and temp is not None:
            print('Temp={0:0.1f}*C Humidity={1:0.1f}%'.format(temp, hum))
            log.write("{0},{1:0.1f},{2:0.1f}\n".format(
                strftime('%m/%d %H:%M:%S'), temp, hum))
        # Sometimes the sensor is unable to get a reading because the CPU
        # was busy, sleep for a second and try again
        else:
            print('Reading failed, polling again')
            time.sleep(1)
