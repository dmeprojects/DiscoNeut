/*Standard libraries*/
#include <stdio.h>
#include <math.h>

#include "inttypes.h"

/*Freertos libraries*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*ESP-IDF libraries*/
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

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

/*BUTTON DEFINES*/
#define MODE_BUTTON_GPIO    8

/*MIC GPIO*/
#define MIC_CLK_GPIO    3
#define MIC_SD_GPIO     10
#define MIC_WS_GPIO     9
#define MIC_EN_GPIO     1

/*MIC BUFFER DEFINES*/
#define MIC_BUFFER_SIZE 64   //Default 128

/*LED VARIABLES*/
led_strip_handle_t led_strip;

/*MIC VARIABLES*/
i2s_chan_handle_t rxHandle;
i2s_chan_config_t i2s_channel_config = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
i2s_std_config_t i2s_config = 
{
    .clk_cfg = 
    {
        .sample_rate_hz = 44100,
        .clk_src = I2S_CLK_SRC_DEFAULT,
        .mclk_multiple = I2S_MCLK_MULTIPLE_384,

    },
    .slot_cfg = 
    {
        .data_bit_width = I2S_DATA_BIT_WIDTH_24BIT,
        .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO,
        .slot_mode = I2S_SLOT_MODE_MONO,
        .slot_mask = I2S_STD_SLOT_LEFT,
        .ws_width = I2S_DATA_BIT_WIDTH_32BIT,
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

//int16_t i2sBuffer[MIC_BUFFER_SIZE];

TaskHandle_t audioReceiveTaskHandle = NULL;

uint8_t lLed = 0;

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

// Function to calculate the RMS amplitude of audio data
/*
    Data from the  i2s interface contains 4 bytes/ sample taken. 
    This array contains also the data from the right channel.  Since we are only
    interested in the left channel, we must filter this right channel out
    The 4 bytes must be converted to a 24 bit value with a max of 0xFF FF FF FF FF FF
*/
int32_t get_volume(uint8_t* data, size_t len)
{
    uint8_t lExtraxtedSample[4] = {0};  //# of incomming bytes
    uint8_t lTempValueSampleCounter = 0;
    //int32_t lVolume = 0;    
    uint32_t i;

    uint32_t lTempValue[len/8];

    int64_t lSquaredSample[len/8];
    int64_t lSumSquared = 0;

    int32_t lTempIntValue = 0;
    int32_t lvolume;
    float lMeanSquared;
    int32_t rmsValue;

    //Divide the len by 8 ( 4 bytes for each channel L & R)
    //For example 128 bytes are 16 samples ( 8 left and 8 right)
    uint32_t lSamplesTaken = len / 8;

    if (len < 1)
    {
        ESP_LOGE(TAG, "Buffer len is 0");
        return 0;
    }

    /*Loop troug the full buffer, but skip the odd datablocks
        Dividing the len by 4 ( 4 bytes/sample, givses the total samples)
    */
    for (i = 0; i < len/4; i+=2)
    {
        //Since we only have one channels ( L) we need only the even blocks (0, 2, 4, ...)
        for ( uint8_t byteCounter = 0; byteCounter < 4; byteCounter++)
        {
            if( i == 0)
            {
                lExtraxtedSample[byteCounter] = data[byteCounter ];
                //lSample = data[ byteCounter];
            }
            else
            {
                lExtraxtedSample[byteCounter] = data[byteCounter + (i * 4)];
                //lSample = data[byteCounter + (i * 4)];
            }            
        }

        lTempValue[lTempValueSampleCounter] = 0;

        //Convert the individual bytes to a uint32_t
        lTempValue[lTempValueSampleCounter] = ( (uint32_t)( lExtraxtedSample[1] << 16 ) | (uint32_t)( lExtraxtedSample[2] << 8 ) | ( lExtraxtedSample[3] ) );

        /*Detect the 2 complement*/
        if (lTempValue[lTempValueSampleCounter] > 0x00800000)
        {
            //lTempValue[lTempValueSampleCounter] |= 0xFF800000; //Set highest byte to one to set the - sign

            //Invert all the bits
            lTempValue[lTempValueSampleCounter] = ~lTempValue[lTempValueSampleCounter];

            //Add one to it
            lTempValue[lTempValueSampleCounter]++; 
        }
        lTempValueSampleCounter++;
    }

    //Square each value
    for ( i = 0; i < lTempValueSampleCounter; i++)
    {
        lTempIntValue = lTempValue[i];
         lSquaredSample[i] = lTempIntValue * lTempIntValue ;
    }

    //Calculate sum of Squared values
    for( i = 0; i < lTempValueSampleCounter; i++)
    {
        lSumSquared += lSquaredSample[i];
    }

    lMeanSquared = (float)(lSumSquared) / lSamplesTaken;

    lvolume = (int32_t)sqrt( lMeanSquared);

    if( lvolume > 0xFFFF000)
    {
        lvolume = 0;
    }

    return lvolume;
}

void audioReceiveTask ( void* pvParams)
{
    //uint8_t lStartAudio;
    size_t bytes_read = 0;
    int32_t volume;
    esp_err_t lEspError = ESP_FAIL;

    int32_t lMicData[MIC_BUFFER_SIZE];

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
        lEspError = i2s_channel_read(rxHandle, lMicData, MIC_BUFFER_SIZE, &bytes_read, portMAX_DELAY);
        if(lEspError != ESP_OK)
        {
            ESP_LOGE(TAG, "failed to read channel with error code: %s", esp_err_to_name(lEspError) );
        }
        else
        {
            //ESP_LOGI(TAG,"Bytes Read: %lu", (uint32_t) bytes_read);
            ESP_LOGI(TAG,"%li", lMicData[0]);

/*              for( unsigned i = 0; i < 20; i++)
            {
                ESP_LOGI(TAG, "Byte [%d]:%d", i, lMicData[i]);
            }  */
        }

        //ESP_LOGI("AudioSample", "Bytes read: %d", (unsigned int) bytes_read);

        // Calculate the volume of the received audio data
        //volume = get_volume(lMicData, bytes_read);
        //ESP_LOGI(TAG, "VOL:%li", volume);

        // Clear the I2S buffer for the next read
        //i2s_zero_dma_buffer(I2S_NUM_0);
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

BaseType_t initMicrophone ( void)
{
    BaseType_t lStatus = pdFALSE;
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
    }

    lEspError = i2s_channel_init_std_mode(rxHandle, &i2s_config);
        if(lEspError != ESP_OK)
    {
        ESP_LOGE("AudioInit", "failed to init channel with error code: %s", esp_err_to_name(lEspError));
    }

    lStatus = xTaskCreate( audioReceiveTask, 
                            "AudioReceiveTask",
                            4096,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            audioReceiveTaskHandle );

    if( lStatus == pdFALSE)
    {
        ESP_LOGE("AudioTask", "Failed to create audio task");
    }

    return lStatus;
}

void app_main() 
{
    uint8_t loopCounter = 0;

    /* Configure the peripheral according to the LED type */
    configure_led();
    initLedPower();

    /*Init I2S interface*/
    initMicrophone();

    ledPower(1);

    while (1) {
        //ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        //blink_led();

        //leds_draw_circle();
        //drawCircle();

        readPattern(0, 100, 100);

        vTaskDelay(pdMS_TO_TICKS(150));
        //run_led();

        //scrollCircle();

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}