#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

#include "esp_random.h"

/*Header includes*/
#include "LED_DEFINES.h"
#include "led_patterns.h"

//#include "rotation.h"
#include "orange_circle.h"
//#include "eva.h"

#define LED_INTENSITY       60
#define TOTAL_LEDS          39
//#define MAX_BAR_HEIGTH      41    //for a single bar
/*
    We have 9 different steps:
    1: only center led on
    2: 1 & center left and right led
    3: 2 & center quarter leds
    4: 3 & Upper and lower center led
    5, 6, 7: 4 & 3 quarter line leds
    8, 9: 5 & 2 outer quarter line leds

    2 Additional steps to have a slight overflow

*/
#define MAX_BAR_HEIGTH      9 + 2   //9 different steps
#define DIMCOEFFICIENT      8   //default 4

extern uint32_t xAudioSample;

extern led_strip_handle_t led_strip;

static void setColor( uint32_t color, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    //Color is a 32 bitvalue that needs to be split up in 3 colors:
    *red = (uint8_t)(color >> 16) / DIMCOEFFICIENT;
    *green = (uint8_t)(color >> 8) / DIMCOEFFICIENT;
    *blue = (uint8_t)(color & 0x000000FF) / DIMCOEFFICIENT;
}

//void leds_draw_circle(void)
int drawCircle ( void )
{
    uint8_t loops;
    uint8_t mainLoops;

    led_strip_clear(led_strip); //clear leds

    //fill circle

    for (mainLoops = 0; mainLoops < 5; mainLoops++)
    {
        for( loops = 0; loops < sizeof(CIRCLE_LEDS); loops++)
        {
            led_strip_set_pixel(led_strip, CIRCLE_LEDS[loops], 0, 2, 2);

            led_strip_refresh(led_strip);

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        for( loops = ( sizeof(CIRCLE_LEDS) + 1); loops > 0; loops--)
        {
            led_strip_set_pixel(led_strip, CIRCLE_LEDS[loops - 1], 0, 0, 0);

            led_strip_refresh(led_strip);

            vTaskDelay(pdMS_TO_TICKS(100));
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return 0;
}

int scrollCircle ( void )
{
    uint8_t loops;
    uint8_t mainLoops;

    led_strip_clear(led_strip);

    for( mainLoops = 0; mainLoops < 20; mainLoops++)
    {
        for( loops = 0; loops < sizeof(CIRCLE_LEDS); loops++)
        {
            if(loops > 0 )
            {
                led_strip_set_pixel(led_strip, CIRCLE_LEDS[loops - 1], 0, 0, 0);    //Set previous LED off
                led_strip_set_pixel(led_strip, CIRCLE_LEDS[loops], 0, LED_INTENSITY, 0);    //Set current LED on
            }
            else
            {
                led_strip_set_pixel(led_strip, CIRCLE_LEDS[sizeof(CIRCLE_LEDS) - 1], 0, 0, 0);    //Set first LED on                
                led_strip_set_pixel(led_strip, CIRCLE_LEDS[loops], 0, LED_INTENSITY, 0);    //Set first LED on
            }
            vTaskDelay(pdMS_TO_TICKS(100));

            led_strip_refresh(led_strip);
        }
    }

    return 0;
}

void readPattern (uint8_t lPattern, uint8_t lRuns, uint32_t lDelayMs)
{
    uint32_t lRunCounter = 0;
    uint32_t lFrameCounter = 0;
    int8_t lLedCounter = 39;
    uint8_t lColorCounter = 3;

    //Select pattern
    
    //load pattern info 

    //Clear all leds
    led_strip_clear(led_strip);

    for( lRunCounter = 0; lRunCounter < lRuns; lRunCounter++)
    {
        //Set leds   
        for (lFrameCounter = 0; lFrameCounter < orange_circleFrames; lFrameCounter++) //for loop number of frames
        {
            //for loop number of leds
            for (lLedCounter = 0; lLedCounter < 39; lLedCounter++)
            {
                led_strip_set_pixel(led_strip, lLedCounter, orange_circle[lFrameCounter][lLedCounter][0], //Set R values on led
                                                            orange_circle[lFrameCounter][lLedCounter][1], //Set G values on led
                                                            orange_circle[lFrameCounter][lLedCounter][2]); //Set B values on led   

            }
            
            led_strip_refresh(led_strip);
            vTaskDelay(pdMS_TO_TICKS(lDelayMs));
        }

    }
}

void drawVuBar ( uint32_t BarHeigth)
{

    //uint8_t LED[MAX_BAR_HEIGTH] = {0};      //Contains the status of the leds
    uint8_t led[TOTAL_LEDS][3];    //First byte contains if the led is on or of, the other 3 bytes contain the LED color value
    uint8_t ledCounter = 0;
    uint8_t ledGreen = 0;
    uint8_t ledRed = 0;
    uint8_t ledBlue = 0;

    uint8_t i;
    uint8_t selectedLed;
    uint8_t loopCounter;    //Counts the phases to set all the colors 

    //Clip LED heigth
    if( BarHeigth > MAX_BAR_HEIGTH)
    {
        BarHeigth = MAX_BAR_HEIGTH;
    }
    //first turn all leds off
    for(ledCounter = 0; ledCounter <= TOTAL_LEDS; ledCounter++)
    {
        led[ledCounter][0] = 0; //turn leds off, the color does not matter.
        led[ledCounter][1] = 0; //turn leds off, the color does not matter.
        led[ledCounter][2] = 0; //turn leds off, the color does not matter.
    } 

    //Set Center led to specific color for when there is no audio data
    setColor(APPLEBLUESEAGREEN, &ledRed, &ledGreen, &ledBlue);
    led[CENTER_LED][0] = ledRed;
    led[CENTER_LED][1] = ledGreen;
    led[CENTER_LED][2] = ledBlue;
    
    //If there is at leat one led on
    if( BarHeigth)
    {
        //Loop trough all the different spots
        for(loopCounter = 1; loopCounter < BarHeigth; loopCounter++)
        {
            switch (loopCounter)
            {
                //only center led on, center led must always be on
                case 1:
                    setColor(PURPLE, &ledRed, &ledGreen, &ledBlue);
                    led[CENTER_LED][0] = ledRed;
                    led[CENTER_LED][1] = ledGreen;
                    led[CENTER_LED][2] = ledBlue;
                break;

                 //LEDS left and right on
                case 2:
                    setColor(PINK, &ledRed, &ledGreen, &ledBlue);
                    led[CENTER_LED - 1][0] = ledRed;
                    led[CENTER_LED - 1][1] = ledGreen;
                    led[CENTER_LED - 1][2] = ledBlue;

                    led[CENTER_LED + 1][0] = ledRed;
                    led[CENTER_LED + 1][1] = ledGreen;
                    led[CENTER_LED + 1][2] = ledBlue;

                break;

                //center upper and lower leds on
                case 3:                
                setColor(LIGHTGREEN, &ledRed, &ledGreen, &ledBlue);
                    led[UPPER_CENTER_LED][0] = ledRed;
                    led[UPPER_CENTER_LED][1] = ledGreen;
                    led[UPPER_CENTER_LED][2] = ledBlue;

                    led[LOWER_CENTER_LED][0] = ledRed;
                    led[LOWER_CENTER_LED][1] = ledGreen;
                    led[LOWER_CENTER_LED][2] = ledBlue;
                break;

                //quarter leds on
                case 4:
                setColor(GREEN, &ledRed, &ledGreen, &ledBlue);
                for(i = 0; i < 4; i++)
                {
                    selectedLed = QUARTER_CIRCLE_LEDS[i];                    
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;
                }
                break;

                case 5:
                //Update center LEDS:
                    setColor(YELLOW, &ledRed, &ledGreen, &ledBlue);
                    selectedLed = UPPER_LINE_LEDS_LEFT[1];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = UPPER_LINE_LEDS_RIGHT[1];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_RIGHT[1];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_LEFT[1];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_LEFT[1];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_RIGHT[1];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;            

                break;

                case 6: 
                    setColor(ORANGE, &ledRed, &ledGreen, &ledBlue);
                    selectedLed = UPPER_LINE_LEDS_LEFT[2];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = UPPER_LINE_LEDS_RIGHT[2];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_RIGHT[2];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_LEFT[2];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_LEFT[2];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_RIGHT[2];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;   
                break;

                case 7:
                    //setColor(ORANGE, &ledRed, &ledGreen, &ledBlue);
                    selectedLed = UPPER_LINE_LEDS_LEFT[3];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = UPPER_LINE_LEDS_RIGHT[3];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_RIGHT[3];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_LEFT[3];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_LEFT[3];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_RIGHT[3];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;   
                break;

                case 8:

                    setColor(RED, &ledRed, &ledGreen, &ledBlue);
                    selectedLed = UPPER_LINE_LEDS_LEFT[4];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = UPPER_LINE_LEDS_RIGHT[4];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_RIGHT[4];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_LEFT[4];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_LEFT[4];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_RIGHT[4];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;
                

                break;

                case 9:

                //setColor(RED, &ledRed, &ledGreen, &ledBlue);
                    selectedLed = UPPER_LINE_LEDS_LEFT[5];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = UPPER_LINE_LEDS_RIGHT[5];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_RIGHT[5];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = CENTER_LINE_LEDS_LEFT[5];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_LEFT[5];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    selectedLed = LOWER_LINE_LEDS_RIGHT[5];  //Update Upper left
                    led[selectedLed][0] = ledRed;
                    led[selectedLed][1] = ledGreen;
                    led[selectedLed][2] = ledBlue;

                    break;
            }
        }
    }

        for (ledCounter = 0; ledCounter < TOTAL_LEDS; ledCounter++)
        {
            led_strip_set_pixel(led_strip, ledCounter, led[ledCounter][0], led[ledCounter][1], led[ledCounter][2]);
        }

        led_strip_refresh(led_strip);
}

void breathing (void)
{
    static uint8_t lBreathingStateMachine = 0;
    uint8_t led[4];     //Led index and color
    //uint8_t previousled[4]; //led index and color
    uint8_t lMaxValue = 0;

    //uint8_t ledArray[39];
    uint8_t i;

    uint8_t lRGB[3] = { 0 };

    uint8_t index = 0;

    srand((unsigned) xAudioSample );    //Init random number

     switch (lBreathingStateMachine)
        {
            case 0: 
            
            for( i = 0; i< 39; i++)
            {
                led_strip_set_pixel(led_strip, i, 0, 0, 0);    //Set all leds off
            }
            led_strip_refresh(led_strip);
            lBreathingStateMachine++;   //Switch to next state    //Select index 

            break;

            case 1: //Select random LED values

            index = (uint8_t)(rand() %40);   //return 
            lBreathingStateMachine++;   //Switch to next state

            led[0] = index;

            lMaxValue = 0;

            for(i = 1; i<4; i++)//Set rnd LED color
            {
                led[i] = (uint8_t)(rand() % 255);
                led[i] = led[i] / DIMCOEFFICIENT;     

                if( led[i] > lMaxValue) //select the highest color value, this is needed for our countdown up/down
                {
                    lMaxValue = led[i];
                }    

                if(i < 2)
                {
                    lRGB[i] = 0;
                }                       
            }

            lBreathingStateMachine++;
            break;

            case 2: //Turn led slowly on

            for(i = 0; i < lMaxValue; i++)
            {
                if( lRGB[0] < led[1])
                {
                    lRGB[0]++;
                }
                if( lRGB[1] < led[2])
                {
                    lRGB[1]++;
                }
                if( lRGB[2] < led[2])
                {
                    lRGB[1]++;
                }                

                led_strip_set_pixel(led[0], i, lRGB[0], lRGB[1], lRGB[2]);    //Set all leds off

                led_strip_refresh(led_strip);

                vTaskDelay(pdMS_TO_TICKS(12));
            }

            lBreathingStateMachine++;

            break;

            case 3:
                lBreathingStateMachine++;
            break;

            case 4: //Turn led slowly off,  we dont have case 4, this allows us to  later on implement a delay on

            for(i = lMaxValue; i > 0 ; i--)
            {
                if( lRGB[0] > 0)
                {
                    lRGB[0]--;
                }
                if( lRGB[1] > 0)
                {
                    lRGB[1]--;
                }
                if( lRGB[2] > 0)
                {
                    lRGB[1]--;
                }                

                led_strip_set_pixel(led[0], i, lRGB[0], lRGB[1], lRGB[2]);    //Set all leds off

                led_strip_refresh(led_strip);

                vTaskDelay(pdMS_TO_TICKS(12));
            }

            lBreathingStateMachine++;

            break;

            case 5: //Use an additional delay

            vTaskDelay(pdMS_TO_TICKS(500));

            lBreathingStateMachine = 0;

            break;
        }                  
}