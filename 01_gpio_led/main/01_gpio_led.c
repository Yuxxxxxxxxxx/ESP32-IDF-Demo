#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED1_GPIO GPIO_NUM_12
#define LED2_GPIO GPIO_NUM_13

void led1_run_task(void)
{
	int gpio_level = 0;

	while(1)
	{
		gpio_level = !gpio_level;
		gpio_set_level(LED1_GPIO, gpio_level);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void led2_run_task(void)
{
	int gpio_level = 0;

	while(1)
	{
		gpio_level = !gpio_level;
		gpio_set_level(LED2_GPIO, gpio_level);
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void app_main(void)
{
	gpio_config_t led_conf = {
		.pin_bit_mask = (1 << LED1_GPIO),
		.pull_up_en   = 1,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.mode         = GPIO_MODE_OUTPUT,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	gpio_config(&led_conf);

	led_conf.pin_bit_mask = (1 << LED2_GPIO);
	gpio_config(&led_conf);

	xTaskCreate((void *)led1_run_task, "led1", 1024 * 2, NULL, 0, NULL);
	xTaskCreate((void *)led2_run_task, "led2", 1024 * 2, NULL, 0, NULL);
}
