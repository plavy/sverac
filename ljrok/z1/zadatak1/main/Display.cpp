#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "Display.h"

Display::Display(int dig_pins[2], int segment_pins[7])
{
    for (int i = 0; i < 2; i++)
    {
        m_dig_pins[i] = (gpio_num_t)dig_pins[i];
        gpio_reset_pin(m_dig_pins[i]);
        gpio_set_direction(m_dig_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(m_dig_pins[i], 0);
    }
    for (int i = 0; i < 7; i++)
    {
        m_segment_pins[i] = (gpio_num_t)segment_pins[i];
        gpio_reset_pin(m_segment_pins[i]);
        gpio_set_direction(m_segment_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(m_segment_pins[i], 0);
    }
}

void Display::select_and_set(int i)
{
    set_digit(m_number[i]);
    gpio_set_level(m_dig_pins[i], 1);
}

void Display::unselect(int i)
{
    gpio_set_level(m_dig_pins[i], 0);
    for (int i = 0; i < 7; i++)
    {
        gpio_set_level(m_segment_pins[i], 0);
    }
}

void Display::set_digit(int integer)
{
    switch (integer)
    {
    case 0:
        gpio_set_level(m_segment_pins[6], 1);
        break;
    case 1:
        gpio_set_level(m_segment_pins[0], 1);
        gpio_set_level(m_segment_pins[3], 1);
        gpio_set_level(m_segment_pins[4], 1);
        gpio_set_level(m_segment_pins[5], 1);
        gpio_set_level(m_segment_pins[6], 1);
        break;
    case 2:
        gpio_set_level(m_segment_pins[2], 1);
        gpio_set_level(m_segment_pins[5], 1);
        break;
    case 3:
        gpio_set_level(m_segment_pins[4], 1);
        gpio_set_level(m_segment_pins[5], 1);
        break;
    case 4:
        gpio_set_level(m_segment_pins[0], 1);
        gpio_set_level(m_segment_pins[3], 1);
        gpio_set_level(m_segment_pins[4], 1);
        break;
    case 5:
        gpio_set_level(m_segment_pins[1], 1);
        gpio_set_level(m_segment_pins[4], 1);
        break;
    case 6:
        gpio_set_level(m_segment_pins[1], 1);
        break;
    case 7:
        gpio_set_level(m_segment_pins[3], 1);
        gpio_set_level(m_segment_pins[4], 1);
        gpio_set_level(m_segment_pins[5], 1);
        gpio_set_level(m_segment_pins[6], 1);
    case 8:
        break;
    case 9:
        gpio_set_level(m_segment_pins[4], 1);
        break;
    default:
        ESP_LOGI(LogName, "ERROR Digit set to %d. Must be 0-9.", integer);
        break;
    }
}

void Display::tick()
{
    for (int i = 0; i < 2; i++)
    {
        select_and_set(i);
        vTaskDelay(20 / portTICK_PERIOD_MS);
        unselect(i);
    }
}

void Display::setNumber(int *integers)
{
    for (int i = 0; i < 2; i++)
    {
        int digit = *(integers + i);
        m_number[i] = digit;
    }
}

int* Display::getNumber()
{
    return m_number;
}