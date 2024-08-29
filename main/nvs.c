#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"

#include "nvs.h"

void nvs_init()
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

esp_err_t nvs_write_int(const char *key, int value)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    nvs_init();

    // Abre o NVS para leitura/escrita
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK)
        return err;

    // Escreve o valor
    err = nvs_set_i32(my_handle, key, value);
    if (err != ESP_OK)
    {
        nvs_close(my_handle);
        return err;
    }

    // Compromete a escrita
    err = nvs_commit(my_handle);

    // Fecha o handle NVS
    nvs_close(my_handle);

    return err;
}

esp_err_t nvs_read_int(const char *key, int *value)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    nvs_init();

    // Abre o NVS para leitura
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK)
        return err;

    // Lê o valor
    err = nvs_get_i32(my_handle, key, value);

    // Fecha o handle NVS
    nvs_close(my_handle);

    return err;
}