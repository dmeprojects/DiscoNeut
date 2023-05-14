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

#define LED_INTENSITY   60

extern led_strip_handle_t led_strip;

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
