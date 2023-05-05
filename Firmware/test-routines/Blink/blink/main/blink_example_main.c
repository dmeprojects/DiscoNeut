/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
/*Standard libraries*/
#include <stdio.h>
#include <math.h>

/*Freertos libraries*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/*ESP-IDF libraries*/
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"

/*System perhiperhals*/
#include "driver/i2s.h"
#include "driver/gpio.h"

/*Custom patterns*/
#include "led_patterns.h"

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
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



/*LED VARIABLES*/
static uint8_t s_led_state = 0;
led_strip_handle_t led_strip;

/*MIC VARIABLES*/
// I2S configuration for ICS-43432 microphone
static const i2s_config_t i2s_config = 
{
    .mode = I2S_MODE_MASTER | I2S_MODE_RX,
    .sample_rate = 16000,
    .bits_per_sample = 24,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB,
    .dma_buf_count = 6,
    .dma_buf_len = 60,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
};
// I2S pin configuration for ICS-43432 microphone
static const i2s_pin_config_t  i2s_pin_config = 
{
    .bck_io_num = MIC_CLK_GPIO,
    .ws_io_num = MIC_WS_GPIO,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = MIC_SD_GPIO,
};

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

static void blink_led(void)
{
    /* If the addressable LED is enabled */
    if (s_led_state) {
        /* Set the LED pixel using RGB from 0 (0%) to 255 (100%) for each color */
        led_strip_set_pixel(led_strip, 0, 16, 16, 16);
        /* Refresh the strip to send data */
        led_strip_refresh(led_strip);
    } else {
        /* Set all LED off to clear all pixels */
        led_strip_clear(led_strip);
    }
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
float get_volume(uint8_t* data, size_t len)
{
    float sum = 0.0;
    for (int i = 0; i < len / 4; i++) {
        int32_t sample = *((int32_t*) &data[i * 4]);
        sum += (float) (sample * sample);
    }
    float rms = sqrtf(sum / (float) (len / 4));
    return rms;
}

void audioReceiveTask ( void* pvParams)
{
    uint8_t lStartAudio;
    size_t bytes_read = 0;
    float volume;

    uint8_t* i2s_buffer = (uint8_t*) malloc(i2s_config.dma_buf_len * 2);

    // Start I2S data reception
    i2s_zero_dma_buffer(I2S_NUM_0);
    i2s_start(I2S_NUM_0);

    for(;;)
    {
        bytes_read = 0;

        i2s_read(I2S_NUM_0, i2s_buffer, i2s_config.dma_buf_len * 2, &bytes_read, portMAX_DELAY);

        // Calculate the volume of the received audio data
        float volume = get_volume(i2s_buffer, bytes_read);

        ESP_LOGE("AudioTask", "Volume: %f", volume);

        // Clear the I2S buffer for the next read
        i2s_zero_dma_buffer(I2S_NUM_0);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

BaseType_t initMicrophone ( void)
{
    BaseType_t lStatus = pdFALSE;

    // Set microphone EN low (to enable mono)
    gpio_reset_pin(MIC_EN_GPIO);
    gpio_set_direction(MIC_EN_GPIO, GPIO_MODE_DEF_OUTPUT);

    gpio_set_level(MIC_EN_GPIO, 0);


    // Initialize I2S with the microphone configuration
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &i2s_pin_config);

    // Allocate a buffer to hold the received audio data
    uint8_t* i2s_buffer = (uint8_t*) malloc(i2s_config.dma_buf_len * 2);

    lStatus = xTaskCreate( audioReceiveTask, 
                            "AudioReceiveTask",
                            2048,
                            NULL,
                            tskIDLE_PRIORITY + 2,
                            audioReceiveTaskHandle );

    if( lStatus == pdFALSE)
    {
        ESP_LOGE("AudioTask", "Failed to create audio task");
    }

    return lStatus;
}

void app_main(void)
{

    uint8_t loopCounter = 0;

    /* Configure the peripheral according to the LED type */
    configure_led();
    initLedPower();

    /*Init I2S interface*/
    initMicrophone();

    ledPower(1);

    while (1) {
        ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        //blink_led();

        //leds_draw_circle();
        drawCircle();

        vTaskDelay(pdMS_TO_TICKS(100));
        //run_led();

        scrollCircle();

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
