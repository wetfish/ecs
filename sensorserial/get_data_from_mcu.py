# Establishes connection with microcontroller, takes sensor readings, 
# and echoes them to terminal (for now)

import sys
import serial
import time

LOG_FN = "log.txt"
arduino = serial.Serial('/dev/ttyUSB0',115200)

def get_poll_interval():
    poll_interval = 3
    if(len(sys.argv) > 1):
        try:
            poll_interval = float(sys.argv[1])
        except:
            print("Invalid argument, using default poll interval.")
    print("Poll interval is", poll_interval)
    return poll_interval

def try_read_serial():
    if(arduino.in_waiting):
        data = arduino.readline().decode('utf-8').rstrip()
        return data

def read_serial():
    while(True):
        data = try_read_serial()
        if(type(data) is str):
            return data

class Ask_For_Data:

    def __init__(self, poll_interval):
        self._last_time = time.perf_counter()
        self._is_first_run = True
        self._poll_interval = poll_interval

    def ask(self):            
        now_time = time.perf_counter()
        if (now_time - self._last_time > self._poll_interval or self._is_first_run):
            if self._is_first_run:
                self._is_first_run = False
            self._last_time = now_time
            arduino.write(bytes("poll", 'utf-8'))
            return True
        else:
            return False

if __name__ == "__main__":
    a = Ask_For_Data(get_poll_interval())
    # overwrite file
    f = open(LOG_FN, "w")
    f.close()

    # wait for MCU
    while(True):
        val = try_read_serial()
        if (val == "ecs"):
            # send start command to MCU
            arduino.write(bytes("go", 'utf-8'))
            break
        time.sleep(.2)

    # get column header labels
    labelstring = read_serial()
    labels = labelstring.split(", ")
    print(labelstring)
    with open(LOG_FN, "w") as log:
        log.write("# " + labelstring + "\n")

    while True:
        # if it's been long enough, ask for another set of readings
        if(a.ask()):
            is_first = True
            for reading in labels:
                value = read_serial()
                if is_first:
                    is_first = False
                else:
                    value = ", " + value
                print(value, end="") # printing the value
                with open(LOG_FN, "a") as log:
                    log.write(value)
            print("")
            with open(LOG_FN, "a") as log:
                log.write("\n")