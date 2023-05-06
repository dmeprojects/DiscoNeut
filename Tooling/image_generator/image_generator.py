import tkinter as tk
from tkinter import *
from tkinter import colorchooser
from PIL import ImageTk
from tkinter.colorchooser import *

# Define a function to change the color of the selected circles
def change_color(color):
    for led in selected_leds:
        canvas.itemconfig(led, fill=color)

# Define a function to handle circle selection
def select_led(event):
    
    #Get tag of the clicked circle
    led_tags = canvas.gettags("current")
    
    print("Current LED tag: ", led_tags)
    
    if len(led_tags) > 0:
        led_tag = led_tags[0]
        canvas.itemconfig(led_tag, width = 3)
        color = askcolor()[1]
        canvas.itemconfig(led_tag, fill = color)
    else:
        print("Failed to get LED tag")
    
def create_led(xPos, yPos):
    lRadius = 20
    lColor = "Black"
    led = canvas.create_oval( xPos - lRadius, yPos - lRadius, xPos + lRadius, yPos + lRadius, fill = lColor )
    canvas.itemconfig(led, tags =(str(led), lColor))
    print(canvas.gettags(led))
    return led

# Create a Tkinter window
root = tk.Tk()

# Load an image using Pillow
image = ImageTk.PhotoImage(file="./Disco-Neut.jpg")

# Create a canvas to display the image and circles
canvas = tk.Canvas(root, width=image.width(), height=image.height())
canvas.pack()

# Display the image on the canvas
canvas.create_image(0, 0, anchor="nw", image=image)

# Draw circles on top of the image
led1 = create_led(100, 100)
led2 = create_led(150, 150)
led3 = create_led(200, 200)

# Bind a function to the circles so that they can be selected
canvas.tag_bind(led1, "<Button-1>", select_led)
canvas.tag_bind(led2, "<Button-1>", select_led)
canvas.tag_bind(led3, "<Button-1>", select_led)

# Create a color palette
palette = tk.Frame(root)
palette.pack()

colors = ["red", "orange", "yellow", "green", "blue", "purple", "pink", "black"]
for color in colors:
    button = tk.Button(palette, bg=color, width=3, command=lambda c=color: change_color(c))
    button.pack(side="left")

# Create a list to keep track of selected circles
selected_leds = []

# Start the Tkinter event loop
root.mainloop()
