#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "Display.h"

Display::Display(int dig_pins[4], int segment_pins[7], int dp, int cln)
{
    for (int i = 0; i < 4; i++)
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
    m_dp = (gpio_num_t)dp;
    gpio_reset_pin(m_dp);
    gpio_set_direction(m_dp, GPIO_MODE_OUTPUT);
    gpio_set_level(m_dp, 1);
    m_cln = (gpio_num_t)cln;
    gpio_reset_pin(m_cln);
    gpio_set_direction(m_cln, GPIO_MODE_OUTPUT);
    gpio_set_level(m_cln, 1);
}

void Display::select_and_set(int i)
{
    set_digit(m_time[i]);
    gpio_set_level(m_dp, 1 - m_dots[i]);
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
    for (int i = 0; i < 4; i++)
    {
        select_and_set(i);
        vTaskDelay(20 / portTICK_PERIOD_MS);
        unselect(i);
    }
    if (m_colon_blink && esp_timer_get_time() - m_colon_last_change > 500000)
    {
        gpio_set_level(m_cln, 1 - gpio_get_level(m_cln));
        m_colon_last_change = esp_timer_get_time();
    }
}

void Display::setTime(int *integers)
{
    for (int i = 0; i < 4; i++)
    {
        int digit = *(integers + i);
        if (digit > 9)
        {
            m_time[i] = 0;
        }
        else
        {
            m_time[i] = digit;
        }
    }
}

int *Display::getTime()
{
    return m_time;
}

void Display::setDot(int index)
{
    for (int i = 0; i < 4; i++)
    {
        m_dots[i] = 0;
    }
    if (index == -1)
    {
        run();
    }
    else
    {
        edit();
        m_dots[index] = 1;
    }
}

void Display::run()
{
    m_colon_blink = 1;
}

void Display::edit()
{
    m_colon_blink = 0;
    gpio_set_level(m_cln, 1);
}