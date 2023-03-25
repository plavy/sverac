#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "CButton.h"

CButton::CButton(int port)
{

    // BOOT is GPIO0 (HIGH when released, LOW when pressed)
    // Config Port on constructor
    m_pinNumber = (gpio_num_t)port;
    m_lastState = 1;
    ESP_LOGI(LogName, "Configure port[%d] to input", port);
    gpio_reset_pin(m_pinNumber);
    /* Set the GPIO as a input */
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);
}

void CButton::tick()
{
    int state = gpio_get_level(m_pinNumber);
    if (state != m_lastState)
    {
        switch (state)
        {
        case 0:
            m_lastPress = esp_timer_get_time();
            break;
        case 1:
            m_lastRelease = esp_timer_get_time();
            if (m_lastRelease - m_lastPress > SHORT_CLICK_AT_MOST)
            {
                longPress();
            }
            else
            {
                singleClick();
            }
            break;
        default:
            break;
        }
    }
    m_lastState = state;
}
