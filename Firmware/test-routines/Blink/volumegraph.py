import serial
import re
import matplotlib.pyplot as plt

# Define the regular expression to extract the volume value from the string
volume_regex = r"AudioTask: Volume: (\d+)"

# Initialize the serial connection to the COM port
ser = serial.Serial('COM5', 115200, timeout=1)

# Initialize the plot
plt.ion()
fig, ax = plt.subplots()
xdata, ydata = [], []
line, = ax.plot(xdata, ydata)

# Continuously read data from the serial port and update the plot
while True:
    # Read a line of text from the serial port
    line = ser.readline().decode('utf-8').rstrip()

    # Extract the volume value from the line using regular expression
    match = re.search(volume_regex, line)
    if match:
        volume = int(match.group(1))
        print(f"Volume: {volume}")

        # Append the volume to the plot data and redraw the plot
        xdata.append(len(ydata))
        ydata.append(volume)
        line.set_data(xdata, ydata)
        ax.relim()
        ax.autoscale_view(True,True,True)
        fig.canvas.draw()
        fig.canvas.flush_events()
