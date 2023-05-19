/*Standard libraries*/
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "inttypes.h"

/*Freertos libraries*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"


/*ESP-IDF libraries*/
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

/* #include "usb/cdc_acm_host.h"
#include "usb/vcp_ch34x.hpp"
#include "usb/vcp_cp210x.hpp"
#include "usb/vcp_ftdi.hpp"
#include "usb/vcp.hpp"
#include "usb/usb_host.h"

using namespace esp_usb;
*/

#include "esp_system.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "esp_timer.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "esp_task_wdt.h"




/*System perhiperhals*/
#include "driver/i2s_std.h"
#include "driver/gpio.h"

/*Custom patterns*/
#include "led_patterns.h"

static const char *TAG = "main";
/*LED DEFINES*/
#define LED_GPIO        2
#define LED_PSU_GPIO    0
#define LED_INTENSITY   10

//#define TOTALLEDS 39  //For a single bar
#define TOTALLEDS 9

/*BUTTON DEFINES*/
#define MODE_BUTTON_GPIO    8

/*MIC GPIO*/
#define MIC_CLK_GPIO    3
#define MIC_SD_GPIO     10
#define MIC_WS_GPIO     9
#define MIC_EN_GPIO     1

//#define NOISELEVEL       3000  //Default for bar with 39 leds
//#define NOISELEVEL      5000

/*MIC BUFFER DEFINES*/
#define MIC_BUFFER_SIZE 64  //Default 128

#define EXAMPLE_ESP_WIFI_SSID "Disconeut"
#define EXAMPLE_ESP_WIFI_PASS "Disconeut"
#define EXAMPLE_ESP_WIFI_CHANNEL   6
#define EXAMPLE_MAX_STA_CONN (3)

void timer_callback( void *param);
static void IRAM_ATTR gpio_interrupt_handler(void *args);

SemaphoreHandle_t xLedUpdateSmpr = NULL;

TaskHandle_t audioReceiveTaskHandle = NULL;
TaskHandle_t LedBreathTaskHandle = NULL;

uint8_t xGlobalStateMachine = 0;
uint8_t lLed = 0;

/*LED VARIABLES*/
led_strip_handle_t led_strip;

uint32_t xAudioSample;
uint8_t xGpioTriggered;

/*MIC VARIABLES*/
i2s_chan_handle_t rxHandle;
i2s_chan_config_t i2s_channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
i2s_std_config_t i2s_config = 
{    
    .clk_cfg = 
    {
        .sample_rate_hz = 8000,
        .clk_src = I2S_CLK_SRC_DEFAULT,
        .mclk_multiple = I2S_MCLK_MULTIPLE_128,
    },
    .slot_cfg = 
    {
        .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
        .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
        .slot_mode = I2S_SLOT_MODE_MONO,
        .slot_mask = I2S_STD_SLOT_LEFT,
        .ws_width = I2S_SLOT_BIT_WIDTH_32BIT,
        .ws_pol = false, 
        .bit_shift = false, 
        .left_align = false, 
        .big_endian = false, 
        .bit_order_lsb = false, 
    },
    .gpio_cfg =
    {
        .mclk = I2S_GPIO_UNUSED,
        .bclk = MIC_CLK_GPIO,
        .ws = MIC_WS_GPIO,
        .dout = I2S_GPIO_UNUSED,
        .din = MIC_SD_GPIO,
        .invert_flags = 
        {
            .mclk_inv = false,
            .bclk_inv = false,
            .ws_inv = false,
        },
    },
};

const esp_timer_create_args_t my_timer_args = {
      .callback = &timer_callback,
      .name = "Mode Timer"};
esp_timer_handle_t timer_handler;

void timer_callback( void *param)
{
    
    //stop timer if needed -> no need for this, this was a one shot
    
    //Read out gpio
    uint32_t lPinlevel = gpio_get_level(MODE_BUTTON_GPIO);

    //if gpio pressed, start timer again
    if( lPinlevel == 0)
    {
        esp_timer_start_once(timer_handler, 15000);

    }
    else
    {
        //Pin has been released
        xGpioTriggered = 1;

        //reenable the interrupt
        gpio_intr_enable(MODE_BUTTON_GPIO);
    }

    //if gpio is released, increment state machine

    //stop timer


}

static void init_timer(void)
{
        ESP_ERROR_CHECK(esp_timer_create(&my_timer_args, &timer_handler));
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                              int32_t event_id, void *event_data)
{
  if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " join, AID=%d",
            MAC2STR(event->mac), event->aid);
  }
  else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
    ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d",
            MAC2STR(event->mac), event->aid);
  }
}

void wifi_init_softap()
{
     ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else /* CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT */
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}


static void initLedPower( void )
{
    gpio_reset_pin(LED_PSU_GPIO);
    gpio_set_direction(LED_PSU_GPIO, GPIO_MODE_DEF_OUTPUT);
}

static void ledPower( unsigned char lStatus)
{
    ESP_LOGI(TAG, "Setting LED PSU power to: %d", lStatus);
    gpio_set_level(LED_PSU_GPIO, lStatus);
}

static void run_led(void)
{
    static uint8_t lLedCounter = 0;
    static uint8_t lLedState = 0;

    switch (lLedState)
    {
        case 0:     //Default state, just turn each led on
        led_strip_set_pixel(led_strip, lLedCounter++, LED_INTENSITY, 0, 0);

        if (lLedCounter > 39)
        {
            lLedCounter = 0;
            lLedState++;
        }
        break;

        case 1: //switch the color on each led

        led_strip_set_pixel(led_strip, lLedCounter++, 0, LED_INTENSITY, 0);

                if (lLedCounter > 39)
        {
            lLedCounter = 0;
            lLedState++;
        }
        break;

        case 2: //switch the color on each led

        led_strip_set_pixel(led_strip, lLedCounter++, 0, 0, LED_INTENSITY);

                if (lLedCounter > 39)
        {
            lLedCounter = 0;
            lLedState++;
        }
        break;
        
        case 3: //reset color

        led_strip_clear(led_strip);

        lLedState = 0;
        break;
    }

    led_strip_refresh(led_strip);
}

static void configure_button (void)
{
    gpio_reset_pin(MODE_BUTTON_GPIO);
    gpio_set_direction(MODE_BUTTON_GPIO, GPIO_MODE_INPUT);
    gpio_set_intr_type(MODE_BUTTON_GPIO, GPIO_INTR_LOW_LEVEL);

    gpio_install_isr_service(0);
    gpio_isr_handler_add( MODE_BUTTON_GPIO, gpio_interrupt_handler, (void *)MODE_BUTTON_GPIO);

}

static void IRAM_ATTR gpio_interrupt_handler(void *args)
{
    uint8_t pinNumber = (uint8_t)args;
    //xQueueSendFromISR(interputQueue, &pinNumber, NULL);

    //Check pinnumber
    if (pinNumber == MODE_BUTTON_GPIO)
    {
        gpio_intr_disable(MODE_BUTTON_GPIO);

        esp_timer_start_once(timer_handler, 15000);
        
        xGpioTriggered = pinNumber;
        //disable pin interrupt
        //Start timer

    }
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = 40, // at least one LED on board
    };
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

void audioReceiveTask ( void* pvParams)
{
    //I2S audio variables
    uint32_t lMicData[MIC_BUFFER_SIZE]; //buffer containint 
    size_t bytes_read = 0;
    esp_err_t lEspError = ESP_FAIL;
    
    //VU meter variables
    const uint8_t lSamples = 64;

    uint32_t lVolArray[64] = {0};
    uint32_t lHeigth;
    uint32_t lLevel = 0;
    uint32_t lMinLevel = 0, lMaxLevel = 0;
    uint32_t lMaxLevelAvg = 0, lMinlevelAvg = 0;
 
    uint8_t i;       
    uint8_t lLedCounter;    
    uint8_t lVolArrayCounter = 0;

    uint32_t noiseLevel = 1200;

    // Start I2S data reception
    lEspError = i2s_channel_enable(rxHandle);
    if(lEspError != ESP_OK)
    {
        ESP_LOGE(TAG, "failed to enable channel with error code: %s", esp_err_to_name(lEspError));
    }

    for(;;)
    {
        bytes_read = 0;

        //i2s_read(I2S_NUM_0, i2s_buffer, i2s_config.dma_buf_len * 2, &bytes_read, portMAX_DELAY);
        lEspError = i2s_channel_read(rxHandle, lMicData, MIC_BUFFER_SIZE * 4, &bytes_read, portMAX_DELAY);
        if(lEspError != ESP_OK)
        {
            ESP_LOGE(TAG, "failed to read channel with error code: %s", esp_err_to_name(lEspError) );
        }

        //Used for 8 bit buffer
        //uint32_t lTempData = lMicData[0] << 24 | lMicData[1] << 16 | lMicData[0] << 8 | lMicData[0];
        //Used for 32 bit buffer

        uint32_t lTempData = lMicData[31];

        uint32_t volume = lTempData >> 8; //Shift 1 byte to the right

        

        if( volume >= 0x00800000)
        {
            volume = volume ^ 0xFFFFFFFF;

            volume++;

            volume = volume & 0x00FFFFFF;
        }  

        xAudioSample = volume;     

        //Remove noise
        
        volume = (volume <= noiseLevel) ? 0: (volume - noiseLevel); 
        //volume = (volume <= NOISELEVEL) ? 0: (volume - NOISELEVEL);        

        //Smooth level
        lLevel = ((lLevel * 7) + volume) >> 3;

        //Calculate LED heigth ( TODO: optimize function)
        uint32_t lTeller = (lLevel - lMinlevelAvg) << 4;    //*16
        uint32_t lDivider = (lMaxLevelAvg - lMinlevelAvg);
        
        //float tempResult = (float)(lTeller) / (float)(lDivider);
        uint32_t tempResult = (lTeller / lDivider);
        //tempResult = (TOTALLEDS + 2) * tempResult;

        tempResult =  tempResult * 11;

        tempResult = tempResult >> 4;   //Divide by 16

        lHeigth = tempResult;



        if( xGlobalStateMachine == 2)
        {

                    //Clip top
            if (lHeigth > TOTALLEDS + 2)
            {
                lHeigth = TOTALLEDS + 2;
            } 

            drawVuBar ( lHeigth);
            
        }

        if( xGlobalStateMachine == 3)
        {
            if (lHeigth > 41)
            {
                lHeigth = 41;
            }

            //Set leds
            for (lLedCounter = 0; lLedCounter < TOTALLEDS; lLedCounter++)
            {
                if( lLedCounter >= lHeigth)
                {
                    led_strip_set_pixel(led_strip, lLedCounter, 0, 0, 0);
                }
                else
                {
                    led_strip_set_pixel(led_strip, lLedCounter, 0, 40, 20);
                }
                
            }
            led_strip_refresh(led_strip);  
        }            
        

        lVolArray[lVolArrayCounter] = volume;
        if(++lVolArrayCounter >= 64)
        {
            lVolArrayCounter = 0;
        }

        //Calculate min/max values
        for(i=0; i<64; i++)
        {
            if( lVolArray[i] < lMinLevel)
            {
                lMinLevel = lVolArray[i];
            }
            else if (lVolArray[i] > lMaxLevel)
            {
                lMaxLevel = lVolArray[i];
            }            
        }

        //Check max level boundaries
        if(lMaxLevel - lMinLevel < (TOTALLEDS + 2) )
        {
            lMaxLevel = lMinLevel + TOTALLEDS + 2;
        }

        //Calculate averages
        lMinlevelAvg = ( lMinlevelAvg * 63 + lMinLevel) >> 6;
        lMaxLevelAvg = ( lMaxLevelAvg * 63 + lMaxLevel) >> 6;

        //No delay needed for now.  This can be implemented to make the visualisation more lazy    
        //vTaskDelay(pdMS_TO_TICKS(15));
    }
}

BaseType_t initMicrophone ( void)
{
    BaseType_t lStatus = pdTRUE;
    esp_err_t lEspError = ESP_FAIL;

    // Set microphone EN low (to enable mono)
    gpio_reset_pin(MIC_EN_GPIO);
    gpio_set_direction(MIC_EN_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_set_level(MIC_EN_GPIO, 0);

    //vTaskDelay(pdMS_TO_TICKS(50));

    // Initialize I2S with the microphone configuration
    lEspError = i2s_new_channel(&i2s_channel_config, NULL, &rxHandle);
    if(lEspError != ESP_OK)
    {
        ESP_LOGE("AudioInit", "failed to add channel with error code: %s", esp_err_to_name(lEspError));
        lStatus = pdFALSE;
    }

    lEspError = i2s_channel_init_std_mode(rxHandle, &i2s_config);
        if(lEspError != ESP_OK)
    {
        ESP_LOGE("AudioInit", "failed to init channel with error code: %s", esp_err_to_name(lEspError));
        lStatus = pdFALSE;
    }

    //Create audio task
    lStatus = xTaskCreate( audioReceiveTask, 
                        "AudioReceiveTask",
                        4096,
                        NULL,
                        tskIDLE_PRIORITY + 1,
                        audioReceiveTaskHandle );
    if( lStatus == pdFALSE)
    {
        ESP_LOGE("AudioTask", "Failed to create audio task");
    }

    return lStatus;
}

void app_main() 
{
    BaseType_t lStatus = pdFALSE;
    uint8_t loopCounter = 0;
    uint8_t i = 0;
    uint8_t lBreathStatus = 0;

    xLedUpdateSmpr = xSemaphoreCreateBinary();

/*     esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_AP");
    wifi_init_softap(); */    

    /* Configure the peripheral according to the LED type */
    configure_led();

    initLedPower();

    /*configure input button*/
    configure_button();

    /*Init I2S interface*/
    initMicrophone();

    ledPower(1);

    //Create audio task
    lStatus = xTaskCreate( breathing, 
                        "LedBreath",
                        512,
                        NULL,
                        tskIDLE_PRIORITY + 1,
                        LedBreathTaskHandle );
    if( lStatus == pdFALSE)
    {
        ESP_LOGE("AudioTask", "Failed to create audio task");
    }

    vTaskSuspend(LedBreathTaskHandle);

    

    init_timer();    

    while (1)
    {

        esp_task_wdt_reset();   //reset task watchdog

        switch(xGlobalStateMachine)
        {
            case 0:
            //LED breathing

            switch ( lBreathStatus)
            {
                case 0:
                    vTaskResume(LedBreathTaskHandle);   //Turen on breathing
                    lBreathStatus++;
                break;

                case 1:

                if( xGpioTriggered)     //Wait for button interrupt
                {
                    xGpioTriggered = 0;
                    lBreathStatus++;
                }
                
                break;

                case 2:

                vTaskSuspend(LedBreathTaskHandle);  //Turen off breathing
                lBreathStatus = 0;
                xGlobalStateMachine++;
                break;
            }

            break;

            case 1:

                if( xGpioTriggered)     //Wait for button interrupt
                {
                    xGpioTriggered = 0;
                    lBreathStatus++;
                }

                readPattern(0, 100, 100);
            break;

            case 2: 

            if( xGpioTriggered)     //Wait for button interrupt
                {
                    xGpioTriggered = 0;
                    lBreathStatus++;
                }
            //vu meter center

            break;

            case 3:

                if( xGpioTriggered)     //Wait for button interrupt
                {
                    xGpioTriggered = 0;
                    lBreathStatus++;
                }
            //idle state for VU meter
            break;

            default:
            xGlobalStateMachine = 0;
        }
        
/*         for(i = 0; i < 11; i++)
        {
            drawVuBar((uint32_t)i);

            vTaskDelay(pdMS_TO_TICKS(200));
        }

        led_strip_clear(led_strip);

        vTaskDelay(pdMS_TO_TICKS(500)); */

        //Read button
        //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        //blink_led();

        //leds_draw_circle();
        //drawCircle();

        //readPattern(0, 100, 100);

        //vTaskDelay(pdMS_TO_TICKS(150));
        //run_led();

        //scrollCircle();

        //vTaskDelay(100 / portTICK_PERIOD_MS);

/*         ESP_LOGI(TAG, "Idle");

        if( xSemaphoreTake(xLedUpdateSmpr, portMAX_DELAY))
        {
            drawVuBar ( xAudioSample);

            xSemaphoreGive(xLedUpdateSmpr);
        } */

        

        //vTaskDelay(pdMS_TO_TICKS(60));


    }
}