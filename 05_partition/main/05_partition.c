#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "esp_partition.h"
#include "spi_flash_mmap.h"
#include "esp_log.h"

static const char *TAG = "partition_example";

// test1: read, erase, write
#if 1
void app_main(void)
{
    const esp_partition_t *partition_ptr = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, "storage");
    assert(partition_ptr!= NULL);

    static char store_data[] = "ESP-IDF Paritition Operations Example (Read, Erase, Write)";
    static char read_data[sizeof(store_data)];

    memset(read_data, 0xFF, sizeof(read_data));
    ESP_ERROR_CHECK(esp_partition_erase_range(partition_ptr, 0, partition_ptr->size));

    ESP_ERROR_CHECK(esp_partition_write(partition_ptr, 0, store_data, sizeof(store_data)));
    ESP_LOGI(TAG, "Written data: %s", store_data);

    ESP_ERROR_CHECK(esp_partition_read(partition_ptr, 0, read_data, sizeof(read_data)));
    ESP_LOGI(TAG, "Read data: %s", read_data);

    ESP_ERROR_CHECK(esp_partition_erase_range(partition_ptr, 0, SPI_FLASH_SEC_SIZE));
    
    memset(store_data, 0xFF, sizeof(read_data));
    ESP_ERROR_CHECK(esp_partition_read(partition_ptr, 0, read_data, sizeof(read_data)));
    assert(memcmp(store_data, read_data, sizeof(read_data)) == 0);

    ESP_LOGI(TAG, "Erased data");

    ESP_LOGI(TAG, "Example end");
}
#endif 

// test2: find
#if 0
// Get the string name of type enum values used in this example
static const char* get_type_str(esp_partition_type_t type)
{
    switch(type) {
        case ESP_PARTITION_TYPE_APP:
            return "ESP_PARTITION_TYPE_APP";
        case ESP_PARTITION_TYPE_DATA:
            return "ESP_PARTITION_TYPE_DATA";
        default:
            return "UNKNOWN_PARTITION_TYPE"; // type not used in this example
    }
}

// Get the string name of subtype enum values used in this example
static const char* get_subtype_str(esp_partition_subtype_t subtype)
{
    switch(subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_NVS:
            return "ESP_PARTITION_SUBTYPE_DATA_NVS";
        case ESP_PARTITION_SUBTYPE_DATA_PHY:
            return "ESP_PARTITION_SUBTYPE_DATA_PHY";
        case ESP_PARTITION_SUBTYPE_APP_FACTORY:
            return "ESP_PARTITION_SUBTYPE_APP_FACTORY";
        case ESP_PARTITION_SUBTYPE_DATA_FAT:
            return "ESP_PARTITION_SUBTYPE_DATA_FAT";
        default:
            return "UNKNOWN_PARTITION_SUBTYPE"; // subtype not used in this example
    }
}

// Find the partition using given parameters
static void find_partition(esp_partition_type_t type, esp_partition_subtype_t subtype, const char* name)
{
    ESP_LOGI(TAG, "Find partition with type %s, subtype %s, label %s...", get_type_str(type), get_subtype_str(subtype),
                    name == NULL ? "NULL (unspecified)" : name);

    const esp_partition_t * part  = esp_partition_find_first(type, subtype, name);

    if (part != NULL) {
        ESP_LOGI(TAG, "\tfound partition '%s' at offset 0x%" PRIx32 " with size 0x%" PRIx32, part->label, part->address, part->size);
    } else {
        ESP_LOGE(TAG, "partition not found!");
    }
}

void app_main(void)
{
    /* First Part - Finding partitions using esp_partition_find_first. */

    ESP_LOGI(TAG, "----------------Find partitions---------------");

    find_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_NVS, NULL);
    //find_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, NULL);
    find_partition(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_PHY, NULL);
    find_partition(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);

    /* Second Part - Iterating over partitions */

    ESP_LOGI(TAG, "----------------Iterate through partitions---------------");

    esp_partition_iterator_t it;

    ESP_LOGI(TAG, "Iterating through app partitions...");
    it = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, NULL);

    // 遍历 APP 分区
    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "\tfound partition '%s' at offset 0x%" PRIx32 " with size 0x%" PRIx32, part->label, part->address, part->size);
    }
    esp_partition_iterator_release(it);

    ESP_LOGI(TAG, "Iteratoring througn data partitions...");
    it = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_ANY, NULL);

    // 遍历 Data 分区
    for (; it != NULL; it = esp_partition_next(it)) {
        const esp_partition_t *part = esp_partition_get(it);
        ESP_LOGI(TAG, "\tfound partition '%s' at offset 0x%" PRIx32 " with size 0x%" PRIx32, part->label, part->address, part->size);
    }   

    // 释放迭代器
    esp_partition_iterator_release(it);

    ESP_LOGI(TAG, "Example end");
}
#endif
