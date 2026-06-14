#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h" // Biblioteca de controle de PWM

#define PIN_VISION 19 // definição do pino de sinal
#define PIN_FIRE_DETECTOR 4 // definição do pino receptor do sensor
#define PIN_MOV_BOMB 18 // definição do pino do servo da bomba

// Pinos do ESP32 conectados ao módulo drive
#define PWM_BOMB 23           // Pino principal (Velocidade por PWM)
#define PWM_BOMB_REFERENCE 22  // Pino de referência  (Mantido em 0)

// Valor da velocidade do motor (0 a 65535 para resolução de 16-bits)
#define SPEED 55000     


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

    ledc_channel_config_t channel = { // config do servo visão
        .gpio_num = PIN_VISION,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel);


     ledc_channel_config_t channel_bomb = {  // config do servo bomba
        .gpio_num = PIN_MOV_BOMB,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_1, 
        .timer_sel = LEDC_TIMER_0, 
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&channel_bomb); 
}

   void servo_update(ledc_channel_t channel, int angle) {  // controle da posição exata do servo via canais  (CHANNEL 1 E 0)
    uint32_t duty = angle_to_duty(angle);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, duty); 
    ledc_update_duty(LEDC_LOW_SPEED_MODE, channel); 
}

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
    ledc_channel_config_t ledc_channel_water = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_3,
        .timer_sel = LEDC_TIMER_0,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = PWM_BOMB,
        .duty = 0 // Inicializa desligado por segurança
    };
    ledc_channel_config(&ledc_channel_water);
}

// Função para definir a força/velocidade da bomba
void BOMB_POWER(uint32_t duty_cycle) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, duty_cycle);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
}


void servo_task(void *pvParameters) { // tarefa de repetição, para monitoramento

    while (1) {

        for (int p = 0; p <= 180; p++ ) {

          while (gpio_get_level(PIN_FIRE_DETECTOR) == 0) { // tem que ser 0 devido a resistencia do sensor
            vTaskDelay(pdMS_TO_TICKS(500)); 
          }
            BOMB_POWER(0); // Desliga a bomba (Velocidade 0)
            servo_update(LEDC_CHANNEL_0, p); // config do servo da visão
            vTaskDelay(pdMS_TO_TICKS(15));

            if (gpio_get_level (PIN_FIRE_DETECTOR) == 0) { // configuração do servo da bomba de água
                servo_update(LEDC_CHANNEL_1, p);
                vTaskDelay(pdMS_TO_TICKS(1500));
                BOMB_POWER(SPEED); 
                vTaskDelay(pdMS_TO_TICKS(500));
              }
        }

        for (int p = 180; p >= 0; p--) {

          while (gpio_get_level(PIN_FIRE_DETECTOR) == 0) { // tem que ser 0 devido a resistencia do sensor
            vTaskDelay(pdMS_TO_TICKS(50)); 
          }
          
            servo_update(LEDC_CHANNEL_0, p); // config do servo da visão
            vTaskDelay(pdMS_TO_TICKS(15));

              if (gpio_get_level (PIN_FIRE_DETECTOR) == 0) { // configuração do servo da bomba de água
                servo_update(LEDC_CHANNEL_1, p); // config do servo da bomba de água
                vTaskDelay(pdMS_TO_TICKS(15));
              }
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
 
    BOMB_CONFIG(); // Inicializa o hardware da bomba
    vTaskDelay(pdMS_TO_TICKS(15));

    sensor_infra_config ();
    vTaskDelay(pdMS_TO_TICKS(15));

    servo_config();
    vTaskDelay(pdMS_TO_TICKS(15));

    xTaskCreate(servo_task, "servo_task", 2048, NULL, 1, NULL);
    vTaskDelay(pdMS_TO_TICKS(15));

} 
