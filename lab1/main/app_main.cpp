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

void single_click_action() {
    ESP_LOGI(TAG, "Single click detected");
}

void long_press_action() {
    ESP_LOGI(TAG, "Long press detected");
}

void led_task(void *parameters)
{
    CLed *led;
    led = (CLed *)parameters;

    while (1)
    {
        // Do tick
        led->tick();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void button_task(void *parameters)
{
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
    button1.attachSingleClick(single_click_action);
    button1.attachLongPress(long_press_action);

    // Create tasks
    ESP_LOGI(TAG, "Creating tasks");

    // xTaskCreate(led_task,     // Task function
    //             "led",     // Name of task in task scheduler
    //             1024 * 5,      // Stack size
    //             (void *)&led1, // Parameter send to function
    //             1,             // Priority
    //             &xHandle);     // task handler
    // ESP_LOGI(TAG, "LED task created.");

    xTaskCreate(button_task,     // Task function
                "button",     // Name of task in task scheduler
                1024 * 5,      // Stack size
                (void *)&button1, // Parameter send to function
                1,             // Priority
                &xHandle);     // task handler
    ESP_LOGI(TAG, "BUTTON task created.");
    
    ESP_LOGI(TAG, "All tasks created");

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
