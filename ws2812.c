#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 25
#define WS2812_PIN 7
#define tempo 400

#define LED_RED 13
#define TIME_LED_RED 100
#define QTD_NUMEROS 10
#define BOTAO_A 5
#define BOTAO_B 6

static volatile uint32_t numero_atual = 0;
static volatile uint32_t last_time_A = 0;
static volatile uint32_t last_time_B = 0;
static volatile bool interrupcao_em_progresso = false;

void piscar_led_vermelho();
static void gpio_irq_handler(uint gpio, uint32_t events);

// Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 0; // Intensidade do vermelho
uint8_t led_g = 0; // Intensidade do verde
uint8_t led_b = 20; // Intensidade do azul

// Buffer para armazenar quais LEDs estão ligados matriz 5x5
bool numeros[10][NUM_PIXELS] = {
    {0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0},

    {0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0, 
    0, 0, 1, 0, 0},

    {0, 1, 1, 1, 0, 
    0, 1, 0, 0, 0, 
    0, 1, 1, 1, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0},

    {0, 1, 0, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 0, 1, 0},

    {0, 1, 1, 1, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0, 
    0, 1, 0, 0, 0, 
    0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0, 
    0, 1, 0, 0, 0, 
    0, 1, 1, 1, 0},

    {0, 1, 0, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 0, 0, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0},

    {0, 1, 1, 1, 0, 
    0, 0, 0, 1, 0, 
    0, 1, 1, 1, 0, 
    0, 1, 0, 1, 0, 
    0, 1, 1, 1, 0},
};

static inline void put_pixel(uint32_t pixel_grb)
{
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b)
{
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}


void set_one_led(bool numero[], uint8_t r, uint8_t g, uint8_t b)
{
    // Define a cor com base nos parâmetros fornecidos
    uint32_t color = urgb_u32(r, g, b);

    // Define todos os LEDs com a cor especificada
    for (int i = 0; i < NUM_PIXELS; i++)
    {
        if (numero[i])
        {
            put_pixel(color); // Liga o LED com um no buffer
        }
        else
        {
            put_pixel(0);  // Desliga os LEDs com zero no buffer
        }
    }
}

int main()
{
    PIO pio = pio0;
    int sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);

    gpio_init(LED_RED);
    gpio_set_dir(LED_RED, GPIO_OUT);

    gpio_init(BOTAO_A);                    
    gpio_set_dir(BOTAO_A, GPIO_IN);        
    gpio_pull_up(BOTAO_A);             
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BOTAO_B);                    
    gpio_set_dir(BOTAO_B, GPIO_IN);        
    gpio_pull_up(BOTAO_B);             
    gpio_set_irq_enabled_with_callback(BOTAO_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    set_one_led(numeros[numero_atual], led_r, led_g, led_b);

    while (1)
    {
        piscar_led_vermelho();    
    }

    return 0;
}

/*  Nome da função: piscar_led_vermelho.
    Funcionalidade: Faz o LED vermelho do LED RGB piscar continuamente 5 vezes por segundo.  */
void piscar_led_vermelho() {
    for (int i = 0; i < 10; i++) {
        gpio_put(LED_RED, !gpio_get(LED_RED));
        sleep_ms(TIME_LED_RED);
    }
}

/*  Nome da função: gpio_irq_handler.
    Funcionalidade: Rotina de interrupção para os botões A e B.
    Botão A: incrementa o número exibido na matriz de LED.
    Botão B: decrementa o número exibido na matriz de LED.  */
void gpio_irq_handler(uint gpio, uint32_t events)
{
    if (interrupcao_em_progresso) {
        return;
    }

    interrupcao_em_progresso = true;

    uint32_t current_time = to_us_since_boot(get_absolute_time());

    if (gpio == BOTAO_A && current_time - last_time_A > 200000 && numero_atual < 9) {               
        set_one_led(numeros[++numero_atual], led_r, led_g, led_b);  
        last_time_A = current_time;      
    } else if (gpio == BOTAO_B && current_time - last_time_B > 200000 && numero_atual > 0) {
        set_one_led(numeros[--numero_atual], led_r, led_g, led_b);
        last_time_B = current_time;   
    }

    interrupcao_em_progresso = false;
}