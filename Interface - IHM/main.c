#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// interface IHM de travamento e destravamento do sistema
#define BUTTON_ON 34
#define DEBOUNCE_TIME 50
#define BUTTON_OFF 35

// pinos de monitoramento
#define PIN_VISION 19
#define PIN_FIRE_DETECTOR 04

// pinos da bomba
/* #define PIN_MOV_BOMB 18    Servo motor de controle
   #define PWM_BOMB 23         PWM sentido horário
   #define PWM_BOMB_REVERSE 22 PWM sentido antihorário
*/

void buttons ()
{
    gpio_config_t io_conf = {           // configuração de pinos para o botão ligar e desligar
        .pin_bit_mask = (1ULL << BUTTON_ON) | (1ULL << BUTTON_OFF),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLDOWN_ENABLE,
    };
      gpio_config(&io_conf);
}

void pin_config ()
{                           // configuração dos pinos do sensor infravermelho, e servo de monitoramento
     gpio_config_t io_confg = {               
        .pin_bit_mask = (1ULL << PIN_FIRE_DETECTOR) | (1ULL << PIN_VISION),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_confg); 
}

// OBS: toda nova configuração de pinos fora destes parametros deve conter letras difentes na tarefa config

void app_main (void)

{
    buttons ();
    pin_config ();

  while (1)
    {
        if (gpio_get_level(BUTTON_ON) == 0)
        {
            gpio_set_level (PIN_FIRE_DETECTOR, 1);
            gpio_set_level (PIN_VISION, 1);
            vTaskDelay(pdMS_TO_TICKS(200));  // debounce fixo
        }
          if (gpio_get_level(BUTTON_OFF) == 0) 
          {
              gpio_set_level (PIN_FIRE_DETECTOR, 0);
              gpio_set_level (PIN_VISION, 0);
              vTaskDelay(pdMS_TO_TICKS(200)); //// debounce fixo
          }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

