#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/list.h"
#include "driver/gpio.h"
#include "esp_attr.h"


#if 0
#define KEY_GPIO GPIO_NUM_9

void app_main(void)
{
    gpio_config_t key_conf = {
        .pin_bit_mask = (1ULL << KEY_GPIO),
       .mode = GPIO_MODE_INPUT,
       .pull_up_en = GPIO_PULLUP_ENABLE,
       .pull_down_en = GPIO_PULLDOWN_DISABLE,
       .intr_type = GPIO_INTR_POSEDGE,
    };

    gpio_config(&key_conf);

    while (1) {
        if (gpio_get_level(KEY_GPIO) == 0)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            while (gpio_get_level(KEY_GPIO) == 0);
            printf("KEY PRESSED\n");
        }
    }
}
#endif

#if 1
#define GPIO_INPUT_IO         9
#define GPIO_INPUT_PIN_SEL    1ULL<<GPIO_INPUT_IO
#define ESP_INTR_FLAG_DEFAULT 0

static QueueHandle_t gpio_evt_queue = NULL;

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void gpio_task_example(void* arg)
{
    uint32_t io_num;
    for(;;) {
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            printf("GPIO[%ld] intr, val: %d\n", (uint32_t)io_num, gpio_get_level(io_num));
        }
    }
}

void app_main(void)
{
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,      // 上升、下降沿都产生中断
        .pin_bit_mask = GPIO_INPUT_PIN_SEL,  //bit mask of the pins, use GPIO0 here
        .mode = GPIO_MODE_INPUT,             //set as input mode
        .pull_up_en = 1,                     //enable pull-up mode
    };
    gpio_config(&gpio_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(gpio_task_example, "gpio_task_example", 2048, NULL, 10, NULL);

    //install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);

    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_INPUT_IO, gpio_isr_handler, (void*) GPIO_INPUT_IO);
}
#endif
