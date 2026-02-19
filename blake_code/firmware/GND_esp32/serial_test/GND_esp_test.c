#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "config.h"

void app_main(void) {
    while (1) {
        uint8_t packet[32];
        int i = 0;

        packet[i++] = 0xFE;
        packet[i++] = 1;      // drone ID
        packet[i++] = 0;      // dest
        packet[i++] = 1;      // PKT_TYPE_TELEMETRY

        float vals[] = {37.7749f, -122.4194f, 100.0f, 1.5f, 2.5f, 90.0f, 12.4f};
        for (int j = 0; j < 7; j++) {
            memcpy(&packet[i], &vals[j], 4);
            i += 4;
        }

        packet[i++] = 8;     // satellites
        packet[i++] = 1;     // armed
        packet[i++] = 0;     // streaming
        packet[i++] = 0xFF;  // end byte

        // Write raw bytes directly
        for (int j = 0; j < i; j++) {
            putchar(packet[j]);
        }
        putchar('\n');
        fflush(stdout);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
