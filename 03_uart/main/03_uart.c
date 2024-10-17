#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"


#define UART1_TX_PIN   (GPIO_NUM_4)
#define UART1_RX_PIN   (GPIO_NUM_5)
#define UART1_RTS_PIN  (UART_PIN_NO_CHANGE)
#define UART1_CTS_PIN  (UART_PIN_NO_CHANGE)

#define UART1_PORT_NUM (UART_NUM_1)

static const int RX_BUF_SIZE = 1024;

static void uart_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_DISABLE,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    ESP_ERROR_CHECK(uart_driver_install(UART1_PORT_NUM, RX_BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(UART1_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART1_PORT_NUM, UART1_TX_PIN, UART1_RX_PIN, UART1_RTS_PIN, UART1_CTS_PIN));
}

// test01 - echo
#if 0
void app_main(void)
{
    uart_init();

    uint8_t *data = (uint8_t *) malloc(RX_BUF_SIZE);
    static const char *TAG = "UART1_EXAMPLE";

    while (1)
    {
        int len = uart_read_bytes(UART1_PORT_NUM, data, RX_BUF_SIZE, 20 / portTICK_PERIOD_MS);
        uart_write_bytes(UART1_PORT_NUM, (const char *)data, len);
        if (len) {
            data[len] = '\0';
            ESP_LOGI(TAG, "Recv str: %s", (char *)data);
        }
    }
}
#endif

// test02 - async_rxtx_task
#if 0
int send_data(const char *log_name, const char *data)
{
    const int len = strlen(data);
    const int tx_bytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(log_name, "Wrote %d bytes", tx_bytes);

    return tx_bytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);

    while (1) {
        send_data(TX_TASK_TAG, "Hello World!");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";
    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);

    uint8_t *data = (uint8_t *)malloc(RX_BUF_SIZE + 1);
    
    while (1) {
        const int rx_bytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rx_bytes > 0) {
            data[rx_bytes] = 0;
            ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", rx_bytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, rx_bytes, ESP_LOG_INFO);
        }
    }

    free(data);
}

void app_main(void)
{
    uart_init();

    xTaskCreate(rx_task, "uart_rx_task", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
    xTaskCreate(tx_task, "uart_tx_task", 2048, NULL, configMAX_PRIORITIES - 1, NULL);
}
#endif

// test03 - uart event
#if 1
static QueueHandle_t uart0_queue;
static const char *TAG = "UART1_EXAMPLE";

#define PATTERN_CHR_NUM    (3) 

static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RX_BUF_SIZE);

    for (;;) {
        // 等待 UART 事件
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            bzero(dtmp, RX_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", UART1_PORT_NUM);
            switch (event.type) {
            // UART 接收数据事件
            /* 我们最好快速处理数据事件，因为数据事件的数量会远超过其他类型的事件。
               如果我们在数据事件上花费太多时间，队列可能会满。 */
            case UART_DATA:
                ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                uart_read_bytes(UART1_PORT_NUM, dtmp, event.size, portMAX_DELAY);
                ESP_LOGI(TAG, "[DATA EVT]:");
                uart_write_bytes(UART1_PORT_NUM, (const char*) dtmp, event.size);
                break;
            // 检测到硬件 FIFO 溢出事件。
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                /* 如果发生 FIFO 溢出，您应该考虑为您的应用程序增加流量控制。 
                   中断服务例程（ISR）已经重置了接收 FIFO，
                   作为示例，我们直接刷新接收缓冲区，以便读取更多数据。 */
                uart_flush_input(UART1_PORT_NUM);
                xQueueReset(uart0_queue);
                break;
            // UART 环形缓冲区满
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                /* 如果缓冲区满了，您应该考虑增加缓冲区大小。
                   作为示例，我们直接刷新接收缓冲区，以便读取更多数据。 */
                uart_flush_input(UART1_PORT_NUM);
                xQueueReset(uart0_queue);
                break;
            // 检测到 UART 接收中断事件
            case UART_BREAK:
                ESP_LOGI(TAG, "uart rx break");
                break;
            // 检测到 UART 奇偶校验错误事件
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            // 检测到 UART 帧错误事件
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            // 模式匹配事件
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART1_PORT_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(UART1_PORT_NUM);
                ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1) {
                    /* 之前曾发生过 UART_PATTERN_DET 事件，但模式位置队列已满，因此无法记录位置。
                       我们应该设置一个更大的队列大小。作为示例，我们在这里直接刷新接收缓冲区。 */
                    uart_flush_input(UART1_PORT_NUM);
                } else { 
                    uart_read_bytes(UART1_PORT_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[PATTERN_CHR_NUM + 1];
                    memset(pat, 0, sizeof(pat));
                    uart_read_bytes(UART1_PORT_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG, "read data: %s", dtmp);
                    ESP_LOGI(TAG, "read pat : %s", pat);
                }
                break;
            // 其它事件
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void app_main(void)
{
    esp_log_level_set(TAG, ESP_LOG_INFO);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    //Install UART driver, and get the queue.
    uart_driver_install(UART1_PORT_NUM, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(UART1_PORT_NUM, &uart_config);

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(UART1_PORT_NUM, UART1_TX_PIN, UART1_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    //Set uart pattern detect function.
    uart_enable_pattern_det_baud_intr(UART1_PORT_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    //Reset the pattern queue length to record at most 20 pattern positions.
    uart_pattern_queue_reset(UART1_PORT_NUM, 20);

    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 2048, NULL, 12, NULL);
}
#endif

