#!/usr/bin/python
import argparse
import csv
import Adafruit_DHT
from time import sleep, strftime 

# Handle parsing command line arguments
def get_args():
    parser = argparse.ArgumentParser(description="Logging script for "
            +"various sensors for the raspberry pi")
    parser.add_argument("GPIOpin", type=int, help="GPIO pin # of sensor")
    parser.add_argument("sensortype", type=str, 
            choices=["dht11","dht22"], help="sensor type")
    parser.add_argument("-f", "--file", type=str,
            help="Optional output file for log, defaults to [sensorType].log")
    return parser.parse_args()

# Returns a nicely formatted datetime for csv output
def format_time():
    return strftime('%m/%d %H:%M:%S')

# Start logging a dht11 sensor
def dht11(pin, outfile):
    with open("dht11.log" if outfile==None else outfile, "a") as log:
        while True:
            hum, temp = Adafruit_DHT.read_retry(Adafruit_DHT.DHT11, pin)
            if hum is not None and temp is not None:
                print('Temp: {0:0.1f}*C Humidity: {1:0.1f}%'.format(temp, hum))
                log.write("{0},{1:0.1f},{2:0.1f}\n".format(format_time(),
                    temp, hum))
            # Sometimes the sensor is unable to get a reading b/c CPU was busy
            # sleep for a second and poll again
            else:
                print('Reading failed, polling again')
                sleep(1)

# Start logging a dht22 sensor
def dht22(pin, outfile):
    with open("dht22.log" if outfile==None else outfile, "a") as log:
        while True:
            hum, temp = Adafruit_DHT.read_retry(Adafruit_DHT.DHT22, pin)
            if hum is not None and temp is not None:
                print('Temp: {0:0.1f}*C Humidity: {1:0.1f}%'.format(temp, hum))
                log.write("{0},{1:0.1f},{2:0.1f}\n".format(format_time(),
                    temp, hum))
            # Sometimes the sensor is unable to get a reading b/c CPU was busy
            # sleep for a second and poll again
            else:
                print('Reading failed, polling again')
                sleep(1)

if __name__ == "__main__":
    # Grab command line arguments
    args = get_args()
    pin = args.GPIOpin
    sensor = args.sensortype
    outfile = args.file
    if sensor == "dht11":
        dht11(pin, outfile)
    elif sensor == "dht22":
        dht22(pin, outfile)
    else:
        print("Sensor type " + sensor + "not supported")
        
