import serial
import re
import matplotlib.pyplot as plt

programRunning = True

# open serial port
ser = serial.Serial('COM5', 115200)

# initialize figure
fig = plt.figure()
ax = fig.add_subplot(1,1,1)
ax.set_xlim([0,100])
ax.set_ylim([0,65535])
line, = ax.plot([],[])

# regular expression to extract volume value
vol_regex = re.compile(r'Vol:(\d+)')

# loop to read data and plot volume values
while programRunning:
    # read data from serial port
    data = ser.readline().decode().strip()

    # extract volume value
    match = vol_regex.search(data)
    if match:
        vol = int(match.group(1))
        print(f"Volume: {vol}")

        # plot volume value
        xdata = list(line.get_xdata())
        ydata = list(line.get_ydata())
        xdata.append(len(xdata))
        ydata.append(vol)
        line.set_xdata(xdata)
        line.set_ydata(ydata)
        plt.draw()
        plt.pause(0.05)
        
        
    # check for user input to quit program
    if plt.waitforbuttonpress(0.001):
        if plt.gcf().canvas.key_press_event:
            if plt.gcf().canvas.key_press_event.key == 'q':
                print("Exiting program...")
                programRunning = False
                break

# close serial port
ser.close()
