#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "DS1307.hpp"

//
#include <driver/i2c.h>
#include <lwip/sockets.h>
#include <stdio.h>
#include <time.h>
//

static const char *TAG = "MAIN";

#define SDA_IO 18
#define SCL_IO 19
#define DS1307_ADDRESS 0x68
#define CLK_SPEED 100000
#define UTC_OFFSET +2


/*
 * The value read from the DS1307 is 7 bytes encoded in BCD:
 * 0 - Seconds - 00-59
 * 1 - Minutes - 00-59
 * 2 - Hours   - 00-23
 * 3 - Day     - 01-07
 * 4 - Date    - 01-31
 * 5 - Month   - 01-12
 * 6 - Year    - 00-99
 *
 */

void writeValue(time_t newTime)
{
    ESP_LOGD(TAG, ">> writeValue: %ld", newTime);
    struct tm tm;
    gmtime_r(&newTime, &tm);
    char buf[30];
    ESP_LOGD(TAG, " - %s", asctime_r(&tm, buf));

    esp_err_t errRc;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    ESP_ERROR_CHECK(i2c_master_start(cmd));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (DS1307_ADDRESS << 1) | I2C_MASTER_WRITE, 1 /* expect ack */));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, 0x0, 1));
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_sec), 1));        // seconds
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_min), 1));        // minutes
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_hour), 1));       // hours
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_wday + 1), 1));   // week day
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_mday), 1));       // date of month
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_mon + 1), 1));    // month
    ESP_ERROR_CHECK(i2c_master_write_byte(cmd, intToBCD(tm.tm_year - 100), 1)); // year
    ESP_ERROR_CHECK(i2c_master_stop(cmd));
    errRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
    if (errRc != 0)
    {
        ESP_LOGE(TAG, "i2c_master_cmd_begin: %d", errRc);
    }
    i2c_cmd_link_delete(cmd);
}

void rtt_task(void *parameters)
{
    DS1307 *rtt = (DS1307 *)parameters;

    while (1)
    {
        struct tm tm;
        tm = rtt->getTime();
        ESP_LOGI(TAG, "Now: %s", timeToString(tm));
        ESP_LOGI(TAG, "UNIX time: %ld", timeToUnix(tm, UTC_OFFSET));
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

TaskHandle_t xHandle = NULL;

// ESP32 main function
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Start MAIN.");

    DS1307 rtt(SDA_IO, SCL_IO, DS1307_ADDRESS, CLK_SPEED);

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

    struct tm tm = rtt.getTime();
    ESP_LOGI(TAG, "Now: %s", timeToString(tm));
    rtt.setRegister(0, 10);
    
    // Main loop
    while (1)
    {
    }
}
