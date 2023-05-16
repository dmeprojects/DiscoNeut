import serial
import re
import matplotlib.pyplot as plt

programRunning = True
xCounter = 0

# open serial port
ser = serial.Serial('COM5', 115200)

# initialize figure
fig = plt.figure()
ax = fig.add_subplot(1,1,1)
ax.set_xlim([0,200])
ax.set_ylim([0,10000])
line, = ax.plot([],[])

# regular expression to extract volume value
vol_regex = re.compile(r'VOL:(\d+)')

# loop to read data and plot volume values
while programRunning:
    # read data from serial port
    data = ser.readline().decode().strip()

    # extract volume value
    match = vol_regex.search(data)
    if match:
        vol = int(match.group(1))
        print(f"Volume: {vol}")
        print(f"Volume Hex: {hex(vol)}")

        # plot volume value
        xdata = list(line.get_xdata())
        ydata = list(line.get_ydata())
        xdata.append(xCounter)
        ydata.append(vol)
        line.set_xdata(xdata)
        line.set_ydata(ydata)
        
        xCounter += 1    

        
        
    # check for user input to quit program
    if plt.waitforbuttonpress(0.01):
        if plt.gcf().canvas.key_press_event:
            if plt.gcf().canvas.key_press_event:
                print("Exiting program...")
                programRunning = False
                break
            
    plt.draw()
    plt.pause(0.05)
    
    if (len(xdata) > 200):
        #xCounter = 0
        #xdata.clear()
        plt.clf()

# close serial port
ser.close()

#close plot
plt.close()
