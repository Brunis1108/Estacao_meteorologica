#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "aht20.h"
#include "bmp280.h"
#include "ssd1306.h"
#include "font.h"
#include <math.h>
#include "web_site.h"
#include "ws2812.pio.h"
#include "hardware/pwm.h"

// Display OLED (I2C)
#define I2C_PORT i2c1  // Porta I2C utilizada
#define I2C_PORT1 i2c0
#define PIN_I2C_SDA 14 // Pino SDA (GPIO 14)
#define PIN_I2C_SCL 15 // Pino SCL (GPIO 15)
#define endereco 0x3C  // Endereço I2C do display
#define I2C_SDA 0                   // 0 ou 2
#define I2C_SCL 1
// LEDs RGB individuais
#define led_red 13   // LED Vermelho (GPIO 13)
#define led_blue 12  // LED Azul (GPIO 12)
#define led_green 11 // LED Verde (GPIO 11)

// Botões
#define buttonA 5 // Botão A (GPIO 5)

#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa

// Configurações PWM
const uint16_t period = 4096;   // Período do PWM (12 bits)
const float divider_pwm = 16.0; // Divisor de frequência



// Buzzer
#define BUZZER_PIN 10 // Pino do buzzer (GPIO 10)
#define REST 0        // Define repouso para o buzzer

// Matriz de LEDs WS2812
#define MATRIX_LED 7  // Pino da matriz (GPIO 7)
#define NUM_PIXELS 25 // 5x5 = 25 LEDs

ssd1306_t ssd;              // Objeto do display
bool buttonA_state = false; // Estado do botão A

// Matriz de LEDs WS2812
PIO pio = pio0;                  // Controlador PIO
uint sm = 0;                     // Máquina de estados
uint32_t led_buffer[NUM_PIXELS]; // Buffer para os LEDs

// Navegação
int linhaEcoluna[2] = {0, 0}; // Posição atual na matriz
int pos_modo = 0;             // Definição + inicialização
char ip_recebido[24] = "";
uint cores[3] = {0x00110000,0x00001100, 0x11000000}; // Vermelho, azul, verde

// Função para calcular a altitude a partir da pressão atmosférica
/* ========== PROTÓTIPOS DE FUNÇÕES ========== */
void led_init();                                                                            // Inicializa LEDs RGB
void button_init(int button);                                                               // Inicializa botões
void display_init();                                                                        // Inicializa display OLED
void debounce(uint gpio, uint32_t events);                                                  // Tratamento de botões
void matrix_init();                                                                         // Inicializa matriz de LEDs
void atualizar_leds(bool limpar);       
void atualizar_dados_nos_leds(int valor_col1,int valor_col2,int valor_col3);                                                    // Atualiza matriz WS2812
void acender_leds(uint32_t matriz[5][5],int leds[][2], int qtd, int pos_cor);
void preencher_matriz(uint32_t matriz[5][5],int linhaAtiva, int colunaAtiva, int cor_led);  // Preenche matriz
void converter_Matriz_em_linha(uint32_t matriz[5][5]);                                      // Converte matriz para buffer linear
void buzzer_init();                                                                         // Inicializa buzzer
void buzzer_play_note(int freq, int duration_ms);                                           // Toca uma nota
double calculate_altitude(double pressure){
    return 44330.0 * (1.0 - pow(pressure / SEA_LEVEL_PRESSURE, 0.1903));
}
// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6

int main(){
    stdio_init_all();
    // Para ser utilizado o modo BOOTSEL com botão B
    button_init(botaoB);
    button_init(buttonA);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &debounce);
    gpio_set_irq_enabled(buttonA, GPIO_IRQ_EDGE_FALL, true);

    display_init();
    led_init(led_red);
    led_init(led_blue);
    led_init(led_green);
    matrix_init();
    buzzer_init();
    // Inicializa o I2C
    i2c_init(I2C_PORT1, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa o BMP280
    bmp280_init(I2C_PORT1);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT1, &params);

    // Inicializa o AHT20
    aht20_reset(I2C_PORT1);
    aht20_init(I2C_PORT1);

    // Estrutura para armazenar os dados do sensor
    AHT20_Data data;
    int32_t raw_temp_bmp;
    int32_t raw_pressure;

    char str_tmp1[20]; // Buffer para armazenar a string
    char str_pres[20]; // Buffer para armazenar a string
    char str_umi[20];  // Buffer para armazenar a string
    char maxmin_temp[30];
    char maxmin_umi[30];
    char maxmin_pres[30];
    init_web_site();
    bool cor = true;
    // char *last_dados_modo = "umi";
    while (1)
    {
        printf("%d", pos_modo);
        if (pos_modo != 0){
            printf("Modo: %s Pos: %d", get_dados.modo, pos_modo);
        }
        // Leitura do BMP280
        bmp280_read_raw(I2C_PORT1, &raw_temp_bmp, &raw_pressure);
        int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
        int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);
        // Cálculo da altitude
        double altitude = calculate_altitude(pressure);
        if(dados.temp > get_dados.min[0] && dados.temp < get_dados.max[0]){
            gpio_put(led_red,false);
            gpio_put(led_green,true);
        }else{
            gpio_put(led_green,false);
            gpio_put(led_red,true);
            buzzer_play_note(1000,200);
        }
          // Leitura do AHT20
        if (!aht20_read(I2C_PORT1, &data))
        {
            printf("Erro na leitura do AHT10!\n\n\n");
        }

        // printf("Pressao = %.3f kPa\n", pressure / 1000.0);
        // printf("Temperatura BMP: = %.2f C\n", temperature / 100.0);
        dados.temp = ((double)temperature / 100.0) + get_dados.ajuste[0];
        // printf("TEMP: %.2f",dados.temp);
        dados.press = ((double)pressure / 1000.0) + get_dados.ajuste[2];
        dados.umi = ((double)data.humidity) + get_dados.ajuste[1];
        // int leds_agua = (int)ceil((qtd_agua * 5.0) / 1000.0);
        int v1=((uint)ceil((dados.temp*5.0)/get_dados.max[0]-get_dados.min[0])),v2=((uint)ceil((dados.umi*5.0)/get_dados.max[1]-get_dados.min[1])),v3=((uint)ceil((dados.press*5.0)/get_dados.max[2]-get_dados.min[2]));
        atualizar_dados_nos_leds(v1,v2,v3);
        sprintf(str_tmp1, "Temp: %.1fC", dados.temp);   // Converte o inteiro em string
        sprintf(str_umi, "Umi: %.1f%%", dados.umi);     // Converte o inteiro em string
        sprintf(str_pres, "Press: %.0fm", dados.press); // Converte o inteiro em string
        sprintf(maxmin_temp, "%.f  %.f", get_dados.max[0], get_dados.min[0]);
        sprintf(maxmin_umi, "%.f  %.f", get_dados.max[1], get_dados.min[1]);
        sprintf(maxmin_pres, "%.f  %.f", get_dados.max[2], get_dados.min[2]);

        if (buttonA_state){
            ssd1306_fill(&ssd, !cor);                     // Limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
            ssd1306_line(&ssd, 3, 15, 123, 15, cor);      // Desenha uma linha
            ssd1306_line(&ssd, 3, 32, 123, 32, cor);
            ssd1306_line(&ssd, 3, 45, 123, 45, cor);
            ssd1306_draw_string(&ssd, "Max  Min", 50, 6);   // Desenha uma string
            ssd1306_draw_string(&ssd, "Temp", 5, 20);       // Desenha uma string
            ssd1306_draw_string(&ssd, "Umi", 6, 35);        // Desenha uma string
            ssd1306_draw_string(&ssd, "Pres", 6, 50);       // Desenha uma string
            ssd1306_draw_string(&ssd, maxmin_temp, 48, 20); // Desenha uma string
            ssd1306_draw_string(&ssd, maxmin_umi, 48, 35);  // Desenha uma string
            ssd1306_draw_string(&ssd, maxmin_pres, 48, 50); // Desenha uma string
            ssd1306_line(&ssd, 40, 15, 40, 60, cor);        // Desenha uma linha vertical
            ssd1306_send_data(&ssd);
        }
        else{
            ssd1306_fill(&ssd, !cor);                     // Limpa o display
            ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor); // Desenha um retângulo
            ssd1306_line(&ssd, 3, 15, 123, 15, cor);      // Desenha uma linha
            ssd1306_draw_string(&ssd, "Estacao", 30, 6);  // Desenha uma string
            ssd1306_draw_string(&ssd, ip_recebido, 6, 20);  // Desenha uma string
            ssd1306_draw_string(&ssd, str_tmp1, 6, 30);   // Desenha uma string
            ssd1306_draw_string(&ssd, str_umi, 6, 40);    // Desenha uma string
            ssd1306_draw_string(&ssd, str_pres, 6, 50);   // Desenha uma string
            ssd1306_send_data(&ssd);                      // Atualiza o display

            // Atualiza o display
        }

        sleep_ms(3000);
    }
    return 0;
}

void debounce(uint gpio, uint32_t events)
{
    static uint32_t last_time = 0;
    uint32_t current_time = to_us_since_boot(get_absolute_time());

    // Debounce - ignora pulsos menores que 200ms
    if (current_time - last_time > 200000)
    {
        last_time = current_time;

        // Botão A - controla LEDs RGB
        if (gpio == buttonA)
        {
            buttonA_state = !buttonA_state;
        }
        if (gpio == botaoB)
        {
            reset_usb_boot(0, 0);
        }

        // Botão B - controla cor da matriz e toca música
    }
}
void led_init(int led)
{
    gpio_init(led);
    gpio_set_dir(led, GPIO_OUT);
    gpio_put(led, false);
}

void button_init(int button)
{
    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);
}

void display_init()
{
    i2c_init(I2C_PORT, 400 * 1000); // I2C a 400kHz
    gpio_set_function(PIN_I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_I2C_SDA);
    gpio_pull_up(PIN_I2C_SCL);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}

void matrix_init(){
    // Configura PIO para controlar os LEDs
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, MATRIX_LED, 800000, false);
    pio_sm_set_enabled(pio, sm, true);
    sleep_ms(50); // Delay para estabilização
}
void atualizar_leds(bool nao_limpar) {
    for (int i = 0; i < 25; i++) {
        pio_sm_put_blocking(pio, sm, nao_limpar?led_buffer[i] : 0x00000000);
        if(!nao_limpar)led_buffer[i] = 0x00000000;
    }
}
void atualizar_dados_nos_leds(int valor_col1,int valor_col2,int valor_col3) {
    uint32_t matriz[5][5] = {0x00000000};           // Matriz 5x5 para mapeamento
    valor_col1 = valor_col1<0?0:valor_col1;
    valor_col2 = valor_col2<0?0:valor_col2;
    valor_col3 = valor_col3<0?0:valor_col3;
    int leds_col1[][2] = {{0,3}, {1,3}, {2,3}, {3,3}, {4,3}};
    int leds_col2[][2] = {{0,2}, {1,2}, {2,2}, {3,2}, {4,2}};
    int leds_col3[][2] = {{0,1}, {1,1}, {2,1}, {3,1}, {4,1}};
    printf("%d %d %d\n",valor_col1,valor_col2,valor_col3);
    acender_leds(matriz, leds_col1, valor_col1, 0);
    acender_leds(matriz, leds_col2, valor_col2, 1);
    acender_leds(matriz, leds_col3, valor_col3, 2);
}
// Preenche matriz com a posição ativa
void acender_leds(uint32_t matriz[5][5],int leds[][2], int qtd, int pos_cor){
    for (size_t i = 0;i < qtd; i++){
        preencher_matriz(matriz,leds[i][0], leds[i][1], cores[pos_cor]);
    }
    atualizar_leds(true);
}
void preencher_matriz(uint32_t matriz[5][5], int linhaAtiva, int colunaAtiva, int cor_led){
    for (int x = 0; x < 5; x++){
        for (int y = 0; y < 5; y++){
            // Ativa apenas o LED na posição atual
            matriz[x][y] = (x==linhaAtiva && y == colunaAtiva) ? cor_led:matriz[x][y];
            // printf("%X\n", matriz[x][y]);
        }
    }
    converter_Matriz_em_linha(matriz); // Converte para o formato linear
}
// Converte matriz 5x5 para buffer linear (com mapeamento serpentina)
void converter_Matriz_em_linha(uint32_t matriz[5][5]){
    int countLeds = 0;
    for (int linha = 0; linha < 5; linha++){
        if (linha % 2 == 0){
            // Linhas pares - ordem inversa
            for (int coluna = 4; coluna >= 0; coluna--){
                led_buffer[countLeds++] = matriz[linha][coluna];
            }
        }else{
            // Linhas ímpares - ordem direta
            for (int coluna = 0; coluna < 5; coluna++){
                led_buffer[countLeds++] = matriz[linha][coluna];
            }
        }
    }
}

void buzzer_init()
{
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
}

void buzzer_play_note(int freq, int duration_ms)
{
    if (freq == REST)
    {
        gpio_put(BUZZER_PIN, 0);
        sleep_ms(duration_ms);
        return;
    }

    // Calcula período e número de ciclos
    uint32_t period_us = 1000000 / freq;
    uint32_t cycles = (freq * duration_ms) / 1000;

    // Gera onda quadrada
    for (uint32_t i = 0; i < cycles; i++)
    {
        gpio_put(BUZZER_PIN, 1);
        sleep_us(period_us / 2);
        gpio_put(BUZZER_PIN, 0);
        sleep_us(period_us / 2);
    }
}

/* Armazenar max e min de pressão, temperatura e umidade separadamente
Mostrar os níveis de temp, umi, pres na matriz em relação ao maximo e minino (/5)
ativar o buzer com pwm e os leds quando ultrapassar as barreiras de maximo e minimo*/
