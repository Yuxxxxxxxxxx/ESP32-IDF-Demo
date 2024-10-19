#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "nvs.h"


void app_main(void)
{
    // 初始化 NVS 分区
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

    printf("\n");
    printf("Opening Non-Volatile Storage (NVS) handle... ");

    nvs_handle handle;
    err = nvs_open("storage", NVS_READWRITE, &handle);  // 打开 NVS 分区
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");

        printf("Reading restart counter from NVS ... ");
        int32_t restart_counter = 0;
        err = nvs_get_i32(handle, "restart_counter", &restart_counter);  // 读取数据
        switch (err) {
            case ESP_OK:
                printf("Done\n");
                printf("Restart counter = %" PRIu32 "\n", restart_counter);
                break;
            case ESP_ERR_NVS_NOT_FOUND:
                printf("The value is not initialized yet!\n");
                break;
            default:
                printf("Error (%s) reading!\n", esp_err_to_name(err));
        }

        printf("Updating restart counter in NVS ... ");
        restart_counter++;
        err = nvs_set_i32(handle, "restart_counter", restart_counter);  // 写入数据
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        printf("Committing updates to NVS ... ");
        err = nvs_commit(handle);  // 更新数据
        printf((err != ESP_OK) ? "Failed!\n" : "Done\n");

        nvs_close(handle);
    }

    printf("\n");

    for (int i = 10; i >= 0; --i) {
        printf("Restarting in %d seconds... \n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now. \n");
    fflush(stdout);
    esp_restart();
}
