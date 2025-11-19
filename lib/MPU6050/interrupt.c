#include "interrupt_api.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

static void IRAM_ATTR gpio_isr_handler(void* args) {
    
    //the procedure sets the flag

}

void interrupt_setup_init() {

    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*) BUTTON_PIN);
}