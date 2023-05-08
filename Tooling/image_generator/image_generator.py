from tkinter import *
from tkinter import colorchooser
from tkinter import filedialog, simpledialog
from PIL import ImageTk
from tkinter.colorchooser import *
import os

class ImageGenerator:
    def __init__(self):
        
        self.programVersion = 0.1
        
        self.tooltip_text = ""
        self.file_path = "coordinates.txt"
        self.index = 0
        self.numberOfLeds = 39
        
        self.frames = 0     #stores the number of frames in the effect
        self.Effect = []    #stores the frameArrays
        self.EffectScaled = []  #Stores the intensity scaled effect
                
        # Create a Tkinter window
        self.root = Tk()
        # Load an image using Pillow
        self.image = ImageTk.PhotoImage(file="./Disco-Neut.jpg")
        
        # Create a canvas to display the image and circles
        self.canvas = Canvas(self.root, width=self.image.width(), height=self.image.height(), name="discoNeut Pattern generator")
        self.canvas.pack()

        # Display the image on the canvas
        self.canvas.create_image(0, 0, anchor="nw", image=self.image)
        
        # Create the tooltip
        self.create_tooltip()
        
        # Get canvas heigth
        self.heigth = self.canvas.winfo_height()
        
        #Create a frame to place on the bottom of the canvas
        self.frame = Frame(self.root)
        self.frame.pack(side = 'bottom')
        
        #create a button in the frame
        self.buttonAddFrame = Button(self.frame, text='Add frame', command=self.add_frame )
        self.buttonAddFrame.pack()
        
        self.buttonGenerateHeader = Button(self.frame, text='Generate header', command=self.generate_headerfile)        
        self.buttonGenerateHeader.pack()
        
        self.buttonGenerateHeader.config(state="disabled")
        
        #create a console output
        self.console = Text(self.root, height=5, bg="white", fg="black", font=("Consolas", 12))
        self.console.pack(side="bottom", fill="both")
        self.console.config(state="disabled")
        
        
        
        # Add a binding to the canvas to show and hide the tooltip
        self.canvas.bind('<Motion>', self.show_tooltip)
        self.canvas.bind('<Leave>', self.hide_tooltip)
        
        # Add a binding to the canvas to export coordinates on spacebar press
        self.root.bind('<space>', self.export_coordinates)
        
        
        # Draw circles on top of the image to mimic the LEDS
        
        #open the LEDLocation file
        with open("LEDLocation.txt", 'r') as f:
            linesInFile = f.readlines()
            f.close()
            
        numberOfLinesInFile = len(linesInFile)
        
        print("Lines in file: " + str(numberOfLinesInFile))

        for i in range(numberOfLinesInFile):
            
            lineContent = linesInFile[i]
            lineParts = lineContent.split(";")   
            index = int(lineParts[0])
            xPos = int(lineParts[1])
            yPos = int(lineParts[2])
            
            #print( "Index: " + str(index) + " xPos: " + str(xPos) + " yPos: " + str(yPos) )
            
            self.ledLabel = self.create_led(xPos, yPos)
            
            #print( "Led label: " + str(self.ledLabel))
            
        self.print_console("FRAME GENERATOR V" + str(self.programVersion))

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
            #print( "Chosen color = " + color)
            self.canvas.itemconfig(led_tag, fill = color)
        else:
            print("ERROR: Failed to get LED tag")
            
    def reset_led(self, event):
        lColor = "#000000"
        returnTag = self.canvas.gettags("current")
        tag = returnTag[0]
        print("LED tag: "+ str(tag))
        self.canvas.itemconfig(tag, fill = lColor)
        
    def create_led(self, xPos, yPos):
        lRadius = 20
        lColor = "Black"
        lFillColor = "#000000"
        led = self.canvas.create_oval( xPos - lRadius, yPos - lRadius, xPos + lRadius, yPos + lRadius, fill = lFillColor )
        self.canvas.itemconfig(led, tags =("led"+str(led), lColor))
        #print(self.canvas.gettags(led))        
        self.canvas.tag_bind(led, "<Button-1>", self.select_led)
        self.canvas.tag_bind(led, "<Button-3>", self.reset_led)
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
        
    def add_frame(self):
        
        frameArray = []    #stores the array with the colors of the individual leds
        frameArrayScaled = []  #stores the intensity scaled effect
        #export colors of all leds
        led_tags = self.canvas.find_withtag("Black")            
        print("tags found: " + str(led_tags))
        
        for tag in led_tags:
            fillColor = self.canvas.itemcget(tag, "fill")
            print(f"Tag: {tag}; color: {fillColor}")
            #self.print_console( (f"Tag: {tag}; color: {fillColor}"))
            r = int(fillColor[1:3], 16)
            g = int(fillColor[3:5], 16)
            b = int(fillColor[5:7], 16)
            
            rScaled = int(r * (60/255))
            gScaled = int(g * (60/255))
            bScaled = int(b * (60/255))
            
            #self.print_console( f"LED:{tag};R{r};G{g};B{b}")
            
            frameArray.append([r,g,b])
            
            frameArrayScaled.append([rScaled,gScaled,bScaled])
            
        #Print array
        self.print_console("Array")
        self.print_console(str(frameArray))
        
        self.print_console("Scaled array:")
        self.print_console(str(frameArrayScaled))
        
        self.Effect.append(frameArray)
        
        self.EffectScaled.append(frameArrayScaled)
        
        print(" Array length = " + str(len(self.Effect)))
        
        self.frames = self.frames + 1
        
        self.print_console("Total number of frames: " + str(self.frames))        
        
        
        
        #enable button when adding at leas one frame to the config
        self.buttonGenerateHeader.config(state="active")
        return 0
        
    def generate_headerfile(self):
        #Generate header file  
        
        filename = filedialog.asksaveasfilename(defaultextension=".h")
        
        if not filename:
            return
        
        patternName = simpledialog.askstring("Pattern Name", "Input a custom name for the pattern:")
        
        if not patternName:
            return
        
        
        if filename:
            with open(filename, "w") as f:
                f.write("#ifndef FRAME_ARRAY_H\n")
                f.write("#define FRAME_ARRAY_H\n\n")                
                f.write("#include <stdint.h>\n\n")                
                f.write("#define NUM_LEDS {}\n".format(len(self.EffectScaled[0])))
                f.write("#define NUM_COLORS 3\n")
                f.write("const uint32_t " + patternName + "Frames = " + str(len(self.EffectScaled)) + ";\n")         
                f.write("const uint8_t " + patternName + "[" + str(len(self.EffectScaled)) + "][NUM_LEDS][NUM_COLORS] = {\n")
                for i, frame in enumerate(self.EffectScaled):
                    f.write("\t{\n")
                    for j, led in enumerate(frame):
                        f.write("\t\t{")
                        f.write(str(led[0]) + ",")
                        f.write(str(led[1]) + ",")
                        f.write(str(led[2]))
                        f.write("}")
                        if j != len(frame)-1:
                            f.write(",")
                        f.write("\n")
                    f.write("\t}")
                    if i != len(self.EffectScaled)-1:
                        f.write(",")
                    f.write("\n")
                f.write("};\n\n")
                f.write("#endif // FRAME_ARRAY_H")
            print("Header file exported to {}".format(filename))

        return 0 
    
    def print_console(self, text):
        self.console.config(state="normal")
        self.console.insert('end', text + "\n")
        self.console.config(state="disabled")
    
            
                
if __name__ == "__main__":
    ImageGenerator()
    
