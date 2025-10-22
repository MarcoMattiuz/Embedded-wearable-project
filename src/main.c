#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BUTTON_GPIO 21  // scegli un pin libero (21 va bene sul DevKitC)
#define LED_GPIO 12

void app_main(void) {
    // gpio_config_t io_conf_IN = {
    //     .pin_bit_mask = (1ULL << BUTTON_GPIO),
    //     .mode = GPIO_MODE_INPUT,
    //     .pull_up_en = 1,      // attiva resistenza di pull-up
    //     .pull_down_en = 0,
    //     .intr_type = GPIO_INTR_DISABLE
    // };
    // gpio_config(&io_conf_IN);

    gpio_config_t io_conf_OUT = {
        .pin_bit_mask = (1ULL << LED_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = 0,      // attiva resistenza di pull-up
        .pull_down_en = 0,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf_OUT);

    printf("Test pulsante su GPIO%d avviato!\n", BUTTON_GPIO);

    while (1) {
        // int stato = gpio_get_level(BUTTON_GPIO);
        // if (stato == 0) {
            gpio_set_level(LED_GPIO, 1);
            // vTaskDelay(pdMS_TO_TICKS(50));
            printf("Pulsante PREMUTO\n");
        // } else {
            gpio_set_level(LED_GPIO, 0);
            // // vTaskDelay(pdMS_TO_TICKS(50));
            printf("Pulsante RILASCIATO\n");
        // }
        // vTaskDelay(pdMS_TO_TICKS(200));
    }
}