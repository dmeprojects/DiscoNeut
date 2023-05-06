from tkinter import *
from tkinter import colorchooser
from PIL import ImageTk
from tkinter.colorchooser import *
import os

class ImageGenerator:
    def __init__(self):
        
        self.tooltip_text = ""
        self.file_path = "coordinates.txt"
        self.index = 0
        
        # Create a Tkinter window
        self.root = Tk()
        # Load an image using Pillow
        self.image = ImageTk.PhotoImage(file="./Disco-Neut.jpg")
        
        # Create a canvas to display the image and circles
        self.canvas = Canvas(self.root, width=self.image.width(), height=self.image.height())
        self.canvas.pack()

        # Display the image on the canvas
        self.canvas.create_image(0, 0, anchor="nw", image=self.image)
        
        # Create the tooltip
        self.create_tooltip()
        
        # Get canvas heigth
        self.heigth = self.canvas.winfo_height()
        
        
        # Add a binding to the canvas to show and hide the tooltip
        self.canvas.bind('<Motion>', self.show_tooltip)
        self.canvas.bind('<Leave>', self.hide_tooltip)
        
        # Add a binding to the canvas to export coordinates on spacebar press
        self.root.bind('<space>', self.export_coordinates)
        
        # Draw circles on top of the image
        self.led1 = self.create_led(100, 100)
        self.led2 = self.create_led(150, 150)
        self.led3 = self.create_led(200, 200)

        # Bind a function to the circles so that they can be selected
        self.canvas.tag_bind(self.led1, "<Button-1>", self.select_led)
        self.canvas.tag_bind(self.led2, "<Button-1>", self.select_led)
        self.canvas.tag_bind(self.led3, "<Button-1>", self.select_led)

        # Start the Tkinter event loop
        self.root.mainloop()    
    

    def create_tooltip(self):
        # Create tooltip window
        self.tip = Toplevel(self.root)
        self.tip.withdraw()
        self.tip.overrideredirect(True)
        self.tip.geometry("+{x}+{y}".format(x=0, y=self.root.winfo_height()))
        
        # Create label for tooltip text
        self.tip_label = Label(self.tip, text=self.tooltip_text, bg='white', padx=5, pady=2)
        self.tip_label.pack()
        
    def show_tooltip(self, event):
        # Update tooltip text and position
        self.tooltip_text = f"X: {event.x}, Y: {event.y}"
        self.tip_label.config(text=self.tooltip_text)
        self.tip.geometry("+{x}+{y}".format(x=event.x_root+10, y=event.y_root+10))
        
        # Show tooltip
        self.tip.deiconify()
    
    def hide_tooltip(self, event):
        # Hide tooltip
        self.tip.withdraw()


    # Define a function to change the color of the selected led
    def change_color(self, color):
        for led in self.selected_leds:
            self.canvas.itemconfig(led, fill=color)

    # Define a function to handle led selection
    def select_led(self, event):
        
        #Get tag of the clicked circle
        led_tags = self.canvas.gettags("current")
        
        print("Current LED tag: ", led_tags)
        
        if len(led_tags) > 0:
            led_tag = led_tags[0]
            self.canvas.itemconfig(led_tag, width = 3)
            color = askcolor()[1]
            self.canvas.itemconfig(led_tag, fill = color)
        else:
            print("Failed to get LED tag")
        
    def create_led(self, xPos, yPos):
        lRadius = 20
        lColor = "Black"
        led = self.canvas.create_oval( xPos - lRadius, yPos - lRadius, xPos + lRadius, yPos + lRadius, fill = lColor )
        self.canvas.itemconfig(led, tags =(str(led), lColor))
        print(self.canvas.gettags(led))
        return led
    
    # Create the coordinates file if it does not exist
    def create_coordinates_file(self):
        if not os.path.exists('coordinates.txt'):
            print("Coordinates file not found, creating now")
            open(self.file_path, 'w').close()
        else:
            print("Coordinates file found, adding new coordinates")
    
    def export_coordinates(self, event):
        
        
             
        self.create_coordinates_file()
        
        print("export coordinates:")
        # Export the coordinates of the current mouse pointer
                
        x = event.x
        y = event.y
        
        with open(self.file_path, "r") as f:
            
            #Get the number of lines in the file
            linesInFile = len(f.readlines())
            
            print("Lines in file: " + str(linesInFile))
            print("Index = " + str(self.index))

        with open(self.file_path, 'a') as f:                
            stringToPrint = str(self.index) + '; X: ' +  str(x) + ' ; Y: ' + str(y)+ '\n'
            print("Writing to file -> " + stringToPrint)
            f.write(stringToPrint)
            f.close()
            
        self.index = self.index + 1
            
                
if __name__ == "__main__":
    ImageGenerator()
    
