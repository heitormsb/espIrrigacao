#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include <stdint.h>

#include "nvs.h"

static const char *TAG = "nvs";

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

esp_err_t nvs_write_int(const char *key, int32_t value)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // nvs_init();

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

esp_err_t nvs_read_int(const char *key, int32_t *value)
{
    nvs_handle_t my_handle;
    esp_err_t err;

    // nvs_init();

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

void write_string_to_nvs(const char *nvs_namespace, const char *key, const char *value){
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre o NVS no modo de leitura/escrita
    err = nvs_open(nvs_namespace, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir NVS: %s", esp_err_to_name(err));
        return;
    }

    // Escreve a string usando a key fornecida
    err = nvs_set_str(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao salvar string (%s) no NVS: %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return;
    }

    // Faz o commit das mudanças
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao confirmar dados no NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "String '%s' salva com sucesso em '%s:%s'", value, nvs_namespace, key);
    }

    // Fecha o handle para liberar recursos
    nvs_close(nvs_handle);
}

int read_wifi_ssid(char *ssid) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre o NVS para leitura
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir o NVS: %s", esp_err_to_name(err));
        return 0;
    }

    // Lê o SSID
    size_t ssid_size = 32;
    err = nvs_get_str(nvs_handle, "ssid", ssid, &ssid_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler o SSID: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return 0;
    }

    // Fecha o NVS
    nvs_close(nvs_handle);
    return 1;
}

int read_wifi_password(char *password) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Abre o NVS para leitura
    err = nvs_open("storage", NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao abrir o NVS: %s", esp_err_to_name(err));
        return 0;
    }

    // Lê a senha
    size_t password_size = 32;
    err = nvs_get_str(nvs_handle, "pass", password, &password_size);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Erro ao ler a senha: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return 0;
    }

    // Fecha o NVS
    nvs_close(nvs_handle);
    return 1;
}


// apagar DEPOIS
void nvs_reset() {
    esp_err_t err;

    // Desmonta e limpa a partição NVS
    err = nvs_flash_deinit();  // Opcional: desmontar antes de apagar
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS já estava desmontado ou erro ao desmontar: %s", esp_err_to_name(err));
    }

    err = nvs_flash_erase();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS apagado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Erro ao apagar NVS: %s", esp_err_to_name(err));
    }

    // Reformatar a partição do NVS
    err = nvs_flash_init();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "NVS reformatado com sucesso!");
    } else {
        ESP_LOGE(TAG, "Erro ao inicializar NVS após apagar: %s", esp_err_to_name(err));
    }
}
