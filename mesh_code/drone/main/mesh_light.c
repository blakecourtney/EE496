#include "esp_err.h"
#include "esp_mesh.h"
#include "mesh_light.h"

esp_err_t mesh_light_init(void) { return ESP_OK; }
esp_err_t mesh_light_set(int color) { return ESP_OK; }
void mesh_connected_indicator(int layer) {}
void mesh_disconnected_indicator(void) {}
esp_err_t mesh_light_process(mesh_addr_t *from, uint8_t *buf, uint16_t len) { return ESP_OK; }