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
int16_t get_volume(int16_t* data, size_t len)
{
    uint32_t lSampleCount = len / (sizeof(uint16_t));
    uint32_t lSampleCounter;

    int64_t lSumSquared = 0;
    int64_t lSquaredSample = 0;

    for( lSampleCounter = 0; lSampleCounter < lSampleCount; lSampleCounter++)
    {
        lSquaredSample = data[lSampleCounter] * data[lSampleCounter];

        lSumSquared += lSquaredSample;
    }

    //Calculate rms
    int16_t lRmsValue = (int16_t)(sqrt(lSumSquared / lSampleCount));

    return lRmsValue;
}

void audioReceiveTask ( void* pvParams)
{
    //uint8_t lStartAudio;
    size_t bytes_read = 0;
    int16_t volume;
    esp_err_t lEspError = ESP_FAIL;

    int16_t lMicData[MIC_BUFFER_SIZE];

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
            //ESP_LOGI(TAG,"%li", lMicData[0]);
        }

        //ESP_LOGI("AudioSample", "Bytes read: %d", (unsigned int) bytes_read);

        // Calculate the volume of the received audio data
        volume = get_volume(lMicData, bytes_read);
        ESP_LOGI(TAG, "VOL:%d", volume);

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