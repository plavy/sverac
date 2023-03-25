#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "CLed.h"
#include "CButton.h"

static const char *TAG = "MAIN";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 2 // CONFIG_BLINK_GPIO
#define BUTTON_GPIO 12

void task_loop(void *parameters)
{
    ESP_LOGI(TAG, "Start TASK Loop.");

    CLed *led;
    led = (CLed *)parameters;

    while (1)
    {
        // Do tick
        led->tick();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void task_button(void *parameters)
{
    ESP_LOGI(TAG, "Start TASK Button.");

    CButton *button;
    button = (CButton *)parameters;
    
    while(1) {
        button->tick();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t xHandle = NULL;

// ESP32 mian function
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Start MAIN.");

    CLed led1(BLINK_GPIO);

    led1.setLedState(LedStatus::BLINK);

    CButton button1(BUTTON_GPIO);

    // Create Task

    ESP_LOGI(TAG, "Start Task Create.");
    xTaskCreate(task_loop,     // Task function
                "ledLoop",     // Name of task in task scheduler
                1024 * 5,      // Stack size
                (void *)&led1, // Parameter send to function
                1,             // Priority
                &xHandle);     // task handler
    ESP_LOGI(TAG, "Task Created.");

    ESP_LOGI(TAG, "Start Task Create.");
    xTaskCreate(task_button,     // Task function
                "buttonLoop",     // Name of task in task scheduler
                1024 * 5,      // Stack size
                (void *)&button1, // Parameter send to function
                1,             // Priority
                &xHandle);     // task handler
    ESP_LOGI(TAG, "Task Created.");

    // Main loop
    while (1)
    {
        led1.setLedState(LedStatus::BLINK);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        led1.setLedState(LedStatus::FAST_BLINK);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        led1.setLedState(LedStatus::SLOW_BLINK);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
