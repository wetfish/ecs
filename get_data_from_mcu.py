# Establishes connection with microcontroller, takes sensor readings, 
# and echoes them to terminal (for now)

import sys
import serial
import time
import os
import argparse

LOG_FN = "log.txt"
arduino = serial.Serial('/dev/ttyUSB0',115200)

def try_read_serial():
    if(arduino.in_waiting):
        data = arduino.readline().decode('utf-8').rstrip()
        return data

def read_serial():
    while(True):
        data = try_read_serial()
        if(type(data) is str):
            return data

def get_next_filename(fn):
    base_fn, ext_fn = os.path.splitext(fn)
    i = 1
    while (os.path.exists(fn)):
        i_str = "_" + str(i)
        fn = base_fn + i_str + ext_fn
        i += 1
    return fn

# poll_intv : time between sensor polls in seconds
# log_fn : filename for sensor log
# overwrite log : bool - either write to same log each time or create new file every time
def get_data_from_MCU(poll_intv, log_fn, overwrite_log):
    a = Ask_For_Data(poll_intv)

    if(not overwrite_log):
        log_fn = get_next_filename(log_fn)
    f = open(log_fn, "w")
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
    with open(log_fn, "w") as log:
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
                with open(log_fn, "a") as log:
                    log.write(value)
            print("")
            with open(log_fn, "a") as log:
                log.write("\n")

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
    ap = argparse.ArgumentParser()
    ap.add_argument("-f", "--filename", type=str, default="log.txt", help="filename for sensor log output file")
    ap.add_argument("-i", "--interval", type=int, default=3, help="time in seconds between sensor polls")
    ap.add_argument("-o", "--overwrite", type=bool, default=False, help="overwrite log file each time or create new")
    args = vars(ap.parse_args())
    get_data_from_MCU(args["interval"], args["filename"], args["overwrite"])
