#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "CButton.h"
#include "dht.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include <math.h>

static const char *TAG = "MAIN";

#define BUTTON_GPIO 12
#define DHT_GPIO 15
#define NTC_CHANNEL ADC_CHANNEL_6 // GPIO34

// DHT22 (AM2301)
static const dht_sensor_type_t sensor_type = DHT_TYPE_AM2301;
static const gpio_num_t dht_gpio = (gpio_num_t)DHT_GPIO;

// ADC1
static esp_adc_cal_characteristics_t *adc_chars;
const float BETA = 3950; // Beta Coefficient of the thermistor

void read_temeperatures()
{
	// DHT
	float humidity;
	float temperature;
	dht_read_float_data(sensor_type, dht_gpio, &humidity, &temperature);
	printf("DHT: Temperature: %f\n", temperature);

	// NTC
	uint32_t adc_reading = 0;
	adc_reading = adc1_get_raw((adc1_channel_t)NTC_CHANNEL);
	uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
	// Rt = Vo * R0 / (Vs - Vo)
	double vs = 3.3;
	double r0 = 10000;
	double vo = voltage / 1000.;
	double rt = vo * r0 / (vs - vo);
	printf("NTC: Raw: %d\tVoltage: %dmV\tResistance: %fOhm\n", adc_reading, voltage, rt);
	// Based on official example: https://wokwi.com/projects/299330254810382858
	double voltage_coefficient = 4; // = adc_reading / analogRead
	float celsius = 1 / (log(1 / (1023. / (adc_reading / voltage_coefficient) - 1)) / BETA + 1.0 / 298.15) - 273.15;
	printf("NTC: Temperature: %f\n", celsius);
	printf("---\n");
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

// ESP32 main function
extern "C" void app_main(void)
{
	ESP_LOGI(TAG, "Start MAIN.");

	// Button setup
	CButton button1(BUTTON_GPIO);
	button1.attachClick(read_temeperatures);

	// ADC1 setup
	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
	adc_chars = (esp_adc_cal_characteristics_t *)calloc(1, sizeof(esp_adc_cal_characteristics_t));
	esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);

	// Create tasks
	ESP_LOGI(TAG, "Creating tasks");

	xTaskCreate(button_task,	  // Task function
				"button",		  // Name of task in task scheduler
				1024 * 5,		  // Stack size
				(void *)&button1, // Parameter send to function
				1,				  // Priority
				&xHandle);		  // task handler
	ESP_LOGI(TAG, "BUTTON task created.");

	ESP_LOGI(TAG, "All tasks created");

	// Main loop
	while (1)
	{
		vTaskDelay(1);
	}
}