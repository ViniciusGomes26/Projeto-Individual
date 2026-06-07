#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h" // Biblioteca de controle de PWM

#define PIN_VISION 19 // definição do pino de sinal
#define PIN_FIRE_DETECTOR 4 // definição do pino receptor do sensor

uint32_t angle_to_duty(int angle) { // definição do limite do giro

    uint32_t min_spin = 500; // 0 Graus
    uint32_t max_spin = 2500; // 180 Graus
    uint32_t pulse = min_spin + ((max_spin - min_spin) * angle) / 180; // Controle de pulso que varia a depender do sentido
    return (pulse * 65535) / 20000; // é um conversor de mS para duty cicle

}

void servo_config() { // configuração do servo via biblioteca Ledc

    ledc_timer_config_t timer = { // controle do PWM
        .speed_mode = LEDC_LOW_SPEED_MODE, 
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .freq_hz = 50,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    ledc_channel_config_t channel = { // controle do microcontrolador
        .gpio_num = PIN_VISION,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);
}

void servo_update(int angle) { // controle da posição exata do servo para o Microcontrolador

    uint32_t duty = angle_to_duty(angle);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty); // encontra a posição
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0); // informa a posição
}

void servo_task(void *pvParameters) { // tarefa de repetição, para monitoramento

    while (1) {

        for (int p = 0; p <= 180; p++ ) {

          while (gpio_get_level(PIN_FIRE_DETECTOR) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50)); 
          }
            servo_update(p);
            vTaskDelay(pdMS_TO_TICKS(15));
        }
        for (int p = 180; p >= 0; p--) {

          while (gpio_get_level(PIN_FIRE_DETECTOR) == 0) {
            vTaskDelay(pdMS_TO_TICKS(50)); 
          }
            servo_update(p);
            vTaskDelay(pdMS_TO_TICKS(15));
        }
    }
}

void sensor_infra_config () {
    
    gpio_config_t io_config = {               
        .pin_bit_mask = (1ULL << PIN_FIRE_DETECTOR),
        .mode = GPIO_MODE_INPUT,
    };
    gpio_config(&io_config); 
}

void app_main (){
 
    sensor_infra_config ();
    servo_config();
    xTaskCreate(servo_task, "servo_task", 2048, NULL, 1, NULL);
}