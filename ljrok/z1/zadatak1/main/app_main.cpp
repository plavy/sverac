#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <rom/ets_sys.h>
#include "esp_log.h"
#include "sdkconfig.h"
#include "Display.h"

#define DATA_IN 4
#define DATA_OUT 2

#define SEGMENT_A 19
#define SEGMENT_B 21
#define SEGMENT_C 32
#define SEGMENT_D 33
#define SEGMENT_E 25
#define SEGMENT_F 27
#define SEGMENT_G 26
#define DIG1 14
#define DIG2 12
#define DP 13

static const char *TAG = "MAIN";

static Display *display;

static int last_shown = 0;

void display_task(void *parameters)
{
    Display *display;
    display = (Display *)parameters;
    while (1)
    {
        display->tick();
    }
}

esp_err_t await_pin_state(gpio_num_t pin, uint32_t timeout,
                          int expected_pin_state, uint32_t *duration)
{
    for (uint32_t i = 0; i < timeout; i += 2)
    {
        ets_delay_us(2);
        if (gpio_get_level(pin) == expected_pin_state)
        {
            if (duration)
                *duration = i;
            return ESP_OK;
        }
    }
    ESP_LOGE(TAG, "FER1 timeout!");
    return ESP_ERR_TIMEOUT;
}

TaskHandle_t xHandle = NULL;

// ESP32 mian function
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Start MAIN.");

    int dig_pins[4] = {DIG1, DIG2};
    int segment_pins[7] = {SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G};
    display = new Display(dig_pins, segment_pins);

    // Create tasks
    ESP_LOGI(TAG, "Creating tasks");

    xTaskCreate(display_task,    // Task function
                "display",       // Name of task in task scheduler
                1024 * 5,        // Stack size
                (void *)display, // Parameter send to function
                1,               // Priority
                &xHandle);       // task handler

    ESP_LOGI(TAG, "All tasks created");

    int temperature_a[2];
    temperature_a[0] = 2;
    temperature_a[1] = 3;
    int humidity_a[2];
    humidity_a[0] = 1;
    humidity_a[1] = 0;

    display->setNumber(temperature_a);

    // init FER1
    gpio_num_t data_out = (gpio_num_t)DATA_OUT;
    gpio_num_t data_in = (gpio_num_t)DATA_IN;
    gpio_set_direction(data_in, GPIO_MODE_OUTPUT);
    gpio_set_direction(data_out, GPIO_MODE_INPUT);

    // Main loop
    while (1)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // send impulse
        gpio_set_level(data_in, 1);
        ets_delay_us(50);
        gpio_set_level(data_in, 0);
        ets_delay_us(100);

        await_pin_state(data_out, 2000, 1, NULL);

        uint32_t temp_duration;
        await_pin_state(data_out, 2000, 0, &temp_duration);

        await_pin_state(data_out, 2000, 1, NULL);

        uint32_t hum_duration;
        await_pin_state(data_out, 2000, 0, &hum_duration);

        int temperature = (temp_duration - 10) / 10;
        ESP_LOGI(TAG, "temp: %d", temperature);
        temperature_a[0] = temperature / 10;
        temperature_a[1] = temperature % 10;
        int humidity = (hum_duration - 10) / 10;
        ESP_LOGI(TAG, "hum: %d", humidity);
        humidity_a[0] = humidity / 10;
        humidity_a[1] = humidity % 10;

        if (last_shown == 0) {
            display->setNumber(temperature_a);
            last_shown = 1;
        } else {
            display->setNumber(humidity_a);
            last_shown = 0;
        }
    }
}
