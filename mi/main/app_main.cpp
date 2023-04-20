#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "CButton.h"
#include "Display.h"

#define DIG1 14
#define DIG2 32
#define DIG3 33
#define DIG4 25
#define SEGMENT_A 21
#define SEGMENT_B 19
#define SEGMENT_C 18
#define SEGMENT_D 5
#define SEGMENT_E 4
#define SEGMENT_F 2
#define SEGMENT_G 15
#define DP 13
#define CLN 12
#define PUSHBUTTON 27

static const char *TAG = "MAIN";

static int current_digit = -1;
static Display *display;

void single_click_action()
{
    ESP_LOGI(TAG, "Single click detected");
    if (current_digit > -1)
    {
        int *digits = (int *)malloc(sizeof(int) * 4);
        digits = display->getTime();
        digits[current_digit] = *(digits + current_digit) + 1;
        display->setTime(digits);
    }
}

void long_press_action()
{
    ESP_LOGI(TAG, "Long press detected");
    ++current_digit;
    if (current_digit > 3)
    {
        current_digit = -1;
    }
    display->setDot(current_digit);
}

void display_task(void *parameters)
{
    Display *display;
    display = (Display *)parameters;
    while (1)
    {
        display->tick();
    }
}

void button_task(void *parameters)
{
    CButton *button;
    button = (CButton *)parameters;

    while (1)
    {
        button->tick();
        vTaskDelay(1);
    }
}

TaskHandle_t xHandle = NULL;

// ESP32 mian function
extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Start MAIN.");

    int dig_pins[4] = {DIG1, DIG2, DIG3, DIG4};
    int segment_pins[7] = {SEGMENT_A, SEGMENT_B, SEGMENT_C, SEGMENT_D, SEGMENT_E, SEGMENT_F, SEGMENT_G};
    display = new Display(dig_pins, segment_pins, DP, CLN);

    CButton button1(PUSHBUTTON);
    button1.attachSingleClick(single_click_action);
    button1.attachLongPress(long_press_action);

    // Create tasks
    ESP_LOGI(TAG, "Creating tasks");

    xTaskCreate(button_task,      // Task function
                "button",         // Name of task in task scheduler
                1024 * 5,         // Stack size
                (void *)&button1, // Parameter send to function
                1,                // Priority
                &xHandle);        // task handler
    ESP_LOGI(TAG, "BUTTON task created.");

    xTaskCreate(display_task,    // Task function
                "display",       // Name of task in task scheduler
                1024 * 5,        // Stack size
                (void *)display, // Parameter send to function
                1,               // Priority
                &xHandle);       // task handler
    ESP_LOGI(TAG, "BUTTON task created.");

    ESP_LOGI(TAG, "All tasks created");

    // Main loop
    while (1)
    {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
