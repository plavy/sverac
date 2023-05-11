#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "DS1307.hpp"
#include <time.h>


static const char *TAG = "MAIN";

#define SDA_IO 18
#define SCL_IO 19
#define DS1307_ADDRESS 0x68
#define CLK_SPEED 100000
#define UTC_OFFSET +2


void rtt_task(void *parameters)
{
    DS1307 *rtt = (DS1307 *)parameters;

    while (1)
    {
        struct tm tm;
        tm = rtt->getTime();
        ESP_LOGI(TAG, "Now: %s", timeToString(tm));
        ESP_LOGI(TAG, "UNIX time: %ld", timeToUnix(tm, UTC_OFFSET));
        int reg = 3;
        ESP_LOGI(TAG, "Register %d: %d", reg, rtt->getRegister(reg));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t xHandle = NULL;

// ESP32 main function
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Start MAIN.");

    DS1307 rtt(SDA_IO, SCL_IO, DS1307_ADDRESS, CLK_SPEED);

    // Test RTT
    ESP_LOGI(TAG, "Now: %s", timeToString(rtt.getTime()));
    rtt.setRegister(2, 5);
    ESP_LOGI(TAG, "Now: %s", timeToString(rtt.getTime()));

    struct tm new_tm;
    new_tm.tm_year = 123; // since 1900
    new_tm.tm_mon = 8; // 0-11
    new_tm.tm_mday = 5;
    new_tm.tm_wday = 2; // 0-6 since Sunday
    new_tm.tm_hour = 1;
    new_tm.tm_min = 15;
    new_tm.tm_sec = 0;
    rtt.setTime(new_tm);
    ESP_LOGI(TAG, "Now: %s", timeToString(rtt.getTime()));

    vTaskDelay(2000 / portTICK_PERIOD_MS);
    
    // Create tasks
    ESP_LOGI(TAG, "Creating tasks");

    xTaskCreate(rtt_task,      // Task function
                "ds1307",         // Name of task in task scheduler
                1024 * 5,         // Stack size
                (void *)&rtt,     // Parameter send to function
                1,                // Priority
                &xHandle);        // task handler
    ESP_LOGI(TAG, "RTT task created.");

    ESP_LOGI(TAG, "All tasks created.");
    
    // Main loop
    while (1)
    {
    }
}
