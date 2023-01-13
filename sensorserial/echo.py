import serial
import time
arduino = serial.Serial('/dev/ttyUSB0',115200)
def read_serial():
    #time.sleep(0.05)
    data = arduino.readline().decode('utf-8').rstrip()
    #.decode('utf-8').rstrip()
    return data
#arduino.write(bytes("2", 'utf-8'))
while True:
    value = read_serial()
    print(value) # printing the value