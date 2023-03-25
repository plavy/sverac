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
    ESP_LOGI(LogName, "Configure port[%d] to input", port);
    gpio_reset_pin(m_pinNumber);
    /* Set the GPIO as a input */
    gpio_set_direction(m_pinNumber, GPIO_MODE_INPUT);
}

void CButton::tick()
{
    ESP_LOGI(LogName, "STATE[%d]", gpio_get_level(m_pinNumber));
}
