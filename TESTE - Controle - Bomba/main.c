#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h" 

// Pinos do ESP32 conectados ao módulo drive
#define PWM_BOMB 23           // Pino principal (Velocidade por PWM)
#define PWM_BOMB_REFERENCE 22  // Pino de referência  (Mantido em 0)

// Valor da velocidade do motor (0 a 65535 para resolução de 16-bits)
#define SPEED 32768           // ~50% de Duty Cycle

void BOMB_CONFIG() {
    
    gpio_config_t io_confh = {               
        .pin_bit_mask = (1ULL << PWM_BOMB_REFERENCE),
        .mode = GPIO_MODE_OUTPUT,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_confh); 
    gpio_set_level(PWM_BOMB_REFERENCE, 0); // Garante o referencial de rotação direta

    // 2. Configura o Timer do PWM para o pino de velocidade
    ledc_timer_config_t timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE, // Nota: LEDC_HIGH_SPEED_MODE foi depreciado em ESP-IDFs recentes
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_16_BIT,
        .freq_hz = 980,                    // Frequência ideal para motores DC menores
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&timer);

    // Configura o Canal PWM associado ao pino principal
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PWM_BOMB,
        .duty = 0 // Inicializa desligado por segurança
    };
    ledc_channel_config(&ledc_channel);
}

// Função para definir a força/velocidade da bomba
void BOMB_POWER(uint32_t duty_cycle) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void app_main() {
  
    // Inicializa o hardware apenas uma vez (Fora do laço infinito)
    BOMB_CONFIG();
  
    while (1) {
        // Liga a bomba com 50% de velocidade
        BOMB_POWER(SPEED); 
        vTaskDelay(pdMS_TO_TICKS(5000)); // Deixa ligada por 5 segundos

        // Desliga a bomba (Velocidade 0)
        BOMB_POWER(0);
        vTaskDelay(pdMS_TO_TICKS(5000)); // Deixa desligada por 5 segundos
    }
}
