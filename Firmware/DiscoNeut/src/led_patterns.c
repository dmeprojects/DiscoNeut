#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

/*Header includes*/
#include "LED_DEFINES.h"
#include "led_patterns.h"

//#include "rotation.h"
#include "orange_circle.h"
//#include "eva.h"

#define LED_INTENSITY       60
#define TOTAL_LEDS          39
#define MAX_BAR_HEIGTH      41
#define DIMCOEFFICIENT      4

extern led_strip_handle_t led_strip;

static void setColor( uint32_t color, uint8_t *red, uint8_t *green, uint8_t *blue)
{
    //Color is a 32 bitvalue that needs to be split up in 3 colors:
    red = (color >> 24) / DIMCOEFFICIENT;
    green = (color >> 16) / DIMCOEFFICIENT;
    blue = (color & 0x000000FF) / DIMCOEFFICIENT;
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
    uint8_t led[MAX_BAR_HEIGTH][3];    //First byte contains if the led is on or of, the other 3 bytes contain the LED color value
    uint8_t ledCounter = 0;
    uint8_t ledGreen = 0;
    uint8_t ledRed = 0;
    uint8_t ledBlue = 0;

    uint8_t i;



/*
            for (lLedCounter = 0; lLedCounter < TOTALLEDS; lLedCounter++)
        {
            if( lLedCounter >= lHeigth)
            {
                led_strip_set_pixel(led_strip, lLedCounter, 0, 0, 0);
            }
            else
            {
                led_strip_set_pixel(led_strip, lLedCounter, 0, 50, 45);
            }
            
        }
        led_strip_refresh(led_strip);
*/

//Clip LED heigth
if( BarHeigth > MAX_BAR_HEIGTH)
{
    BarHeigth = MAX_BAR_HEIGTH;
}

//Map leds to center
switch (BarHeigth)
{
    //All leds off
    if (BarHeigth < 1)
    {
        for(ledCounter = 0; ledCounter < MAX_BAR_HEIGTH; ledCounter++)
        {
            led[ledCounter][ledCounter] = 0; //turn leds off, the color does not matter.
            led[ledCounter][ledCounter + 1] = 0; //turn leds off, the color does not matter.
            led[ledCounter][ledCounter + 2] = 0; //turn leds off, the color does not matter.
        } 
    }

    //only center led on, center led must always be on
    if( BarHeigth < 2)
    {
        setColor(LIGHTGREEN, ledRed, ledGreen, ledBlue);
        led[CENTER_LED][1] = ledRed;
        led[CENTER_LED][2] = ledGreen;
        led[CENTER_LED][3] = ledBlue;
    }

     //LEDS left and right on
     if(BarHeigth < 3)
     {
        for(i = CENTER_LED - 1; i <= CENTER_LED + 1; i++)
        {
            setColor(GREEN, ledRed, ledGreen, ledBlue);
            led[i][1] = ledRed;
            led[i][2] = ledGreen;
            led[i][3] = ledBlue;
        }
     }

    //upper and lower center leds on
     if( BarHeigth < 4)
     {
        for(i = CENTER_LED - 7; i <= CENTER_LED + 7; i+7)
        {
            setColor(GREEN, ledRed, ledGreen, ledBlue);
            led[i][1] = ledRed;
            led[i][2] = ledGreen;
            led[i][3] = ledBlue;
        }
     }

         //upper and lower center leds on
     if( BarHeigth >= 4)
     {
        for(i = CENTER_LED - 7; i <= CENTER_LED + 7; i+7)
        {
            setColor(GREEN, ledRed, ledGreen, ledBlue);
            led[i][1] = ledRed;
            led[i][2] = ledGreen;
            led[i][3] = ledBlue;
        }
     }


   


    
    case 1:

    
    break;
    
    //LEDS left and right on
    case 2:

    break;

    //LEDS left right and top and bottom
    case 3:

    break;
    
    //Also sides from neut
    default:

    break;



}



}
