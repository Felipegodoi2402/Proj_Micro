#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"

// Definição dos pinos do LCD
#define LCD_RS 10 // GP0
#define LCD_E  11 // GP1
#define LCD_D4 2 // GP2
#define LCD_D5 3 // GP3
#define LCD_D6 4 // GP4
#define LCD_D7 5 // GP5

// Funções para controle do LCD
void lcd_send_nibble(uint8_t nibble, bool is_command) {
    gpio_put(LCD_RS, is_command ? 0 : 1); // RS: 0 para comando, 1 para dados
    gpio_put(LCD_D4, (nibble >> 0) & 1);
    gpio_put(LCD_D5, (nibble >> 1) & 1);
    gpio_put(LCD_D6, (nibble >> 2) & 1);
    gpio_put(LCD_D7, (nibble >> 3) & 1);
    gpio_put(LCD_E, 1); // Enable alto
    sleep_us(1);        // Pulso de enable
    gpio_put(LCD_E, 0); // Enable baixo
    sleep_us(100);      // Aguarda o LCD processar
}

void lcd_send_byte(uint8_t byte, bool is_command) {
    lcd_send_nibble(byte >> 4, is_command); // Envia nibble superior
    lcd_send_nibble(byte & 0x0F, is_command); // Envia nibble inferior
}

void lcd_init() {
    // Inicializa os pinos do LCD como saídas
    gpio_init(LCD_RS); gpio_set_dir(LCD_RS, GPIO_OUT);
    gpio_init(LCD_E);  gpio_set_dir(LCD_E, GPIO_OUT);
    gpio_init(LCD_D4); gpio_set_dir(LCD_D4, GPIO_OUT);
    gpio_init(LCD_D5); gpio_set_dir(LCD_D5, GPIO_OUT);
    gpio_init(LCD_D6); gpio_set_dir(LCD_D6, GPIO_OUT);
    gpio_init(LCD_D7); gpio_set_dir(LCD_D7, GPIO_OUT);

    // Inicialização do LCD no modo 4 bits (conforme datasheet HD44780)
    sleep_ms(50); // Aguarda o LCD estabilizar
    lcd_send_nibble(0x03, true); sleep_ms(5);
    lcd_send_nibble(0x03, true); sleep_us(100);
    lcd_send_nibble(0x03, true); sleep_us(100);
    lcd_send_nibble(0x02, true); // Modo 4 bits
    lcd_send_byte(0x28, true); // 4 bits, 2 linhas, fonte 5x8
    lcd_send_byte(0x0C, true); // Display on, cursor off, blink off
    lcd_send_byte(0x06, true); // Incrementa cursor, sem deslocamento
    lcd_send_byte(0x01, true); // Limpa o display
    sleep_ms(2); // Aguarda o clear
}

void lcd_clear() {
    lcd_send_byte(0x01, true); // Comando para limpar
    sleep_ms(2);
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
    uint8_t address = (row == 0) ? 0x80 : 0xC0;
    address += col;
    lcd_send_byte(address, true);
}

void lcd_print(const char *str) {
    while (*str) {
        lcd_send_byte(*str++, false); // Envia caractere como dado
    }
}

int main() {
    stdio_init_all();
    
    // Inicializa o ADC
    adc_init();
    adc_gpio_init(26); // Configura GPIO 26 (ADC0)
    adc_select_input(0); // Seleciona canal ADC0

    // Inicializa o LCD
    lcd_init();

    char buffer[16]; // Buffer para formatar strings

    while (true) {
        // Lê o valor do ADC
        uint16_t adc_value = adc_read();
        float voltage = (adc_value / 4095.0) * 3.3;

        // Mapeia a tensão para o nível do tanque
        float level_percent;
        float low_level = 0.17;
        float high_level = 1.6;
        if (voltage <= low_level) {
            level_percent = 0.0;
        } else if (voltage >= high_level) {
            level_percent = 100.0;
        } else {
            level_percent = ((voltage - low_level) / (high_level - low_level)) * 100.0;
        }

        // Limpa o LCD
        lcd_clear();

        // Exibe a tensão na primeira linha
        lcd_set_cursor(0, 0);
        snprintf(buffer, sizeof(buffer), "Tensao: %.2f V", voltage);
        lcd_print(buffer);

        // Exibe o nível do tanque na segunda linha
        lcd_set_cursor(1, 0);
        snprintf(buffer, sizeof(buffer), "Nivel: %.1f%%", level_percent);
        lcd_print(buffer);

        // Exibe no terminal serial para depuração
        printf("ADC Value: %d, Voltage: %.2f V, Tank Level: %.1f%%\n", 
               adc_value, voltage, level_percent);

        sleep_ms(1000); // Atualiza a cada 1 segundo
    }
}