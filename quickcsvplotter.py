# Plots data from csv file.
# Plots second column of csv on y-axis vs index on x-axis

import csv
import sys
import os
from matplotlib import pyplot as plt # https://matplotlib.org/stable/

# Change these if you want to label plots
PLOT_TITLE = ""
PLOT_Y_LABEL = ""

def get_csvs():
    fns = []
    # no cl argument - load all csvs in current dir
    if len(sys.argv) < 2:
        for filename in os.listdir(os.getcwd()):
            if filename.endswith(".csv"):
                fns.append(filename)
    # otherwise, use the ones provided in command line
    else:
        for arg in sys.argv:
            if arg.endswith(".csv"):
                fns.append(arg)
    return fns

if __name__ == "__main__":
    # get list of csvs - (you can just put filenames in a list and do it that way too if you want)
    csvs = get_csvs()
    #csvs = {'24hour-dht22-test.csv', '24hour-test-dht11.csv', '24hour-test-ds18b20.csv'}
    if len(csvs) < 1:
        sys.exit("No csv files found.")
       
    # Create figure and format it
    fig = plt.figure(dpi = 128, figsize = (10,10))
    plt.title(PLOT_TITLE, fontsize = 24)
    plt.xlabel('',fontsize = 16)
    plt.ylabel(PLOT_Y_LABEL, fontsize = 16)
    plt.tick_params(axis = 'both', which = 'major' , labelsize = 16)
    subplot_num = 0

    # open csvs and plot them
    for csv_file in csvs:
        subplot_num = subplot_num + 1
        with open(csv_file) as f:
            reader = csv.reader(f)
            vals = []
            for col in reader:
                if col[1]=='':
                    continue
                val = float(col[1])
                vals.append(val)  
            
            #Plot Data
            plt.subplot(len(csvs),1,subplot_num)
            plt.title(csv_file)
            plt.plot(vals, c = 'red')

    plt.tight_layout()
    plt.show()