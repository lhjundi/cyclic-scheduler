/**
 * ------------------------------------------------------------
 *  Arquivo: main.c
 *  Projeto: TempCycleDMA
 * ------------------------------------------------------------
 *  Descrição:
 *      Ciclo principal do sistema embarcado, baseado em um
 *      executor cíclico com 3 tarefas principais:
 *
 *      Tarefa 1 - Leitura da temperatura via DMA (meio segundo)
 *      Tarefa 2 - Exibição da temperatura e tendência no OLED
 *      Tarefa 3 - Análise da tendência da temperatura
 *
 *      O sistema utiliza watchdog para segurança, terminal USB
 *      para monitoramento e display OLED para visualização direta.
 *
 *
 *  Data: 12/05/2025
 * ------------------------------------------------------------
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

#include "setup.h"
#include "tarefa1_temp.h"
#include "tarefa2_display.h"
#include "tarefa3_tendencia.h"
#include "tarefa4_controla_neopixel.h"
#include "neopixel_driver.h"
#include "testes_cores.h"
#include "pico/stdio_usb.h"

void task_3_thermal_trend();
void task_5_alert_neopixel();
void task_4_update_neopixel_matrix();
void task_2_show_oled();
void task_1_read_temperature();

float media;
tendencia_t t;
absolute_time_t ini_tarefa1, fim_tarefa1, ini_tarefa2, fim_tarefa2, ini_tarefa3, fim_tarefa3, ini_tarefa4, fim_tarefa4;

int main()
{

        setup(); // Inicializações: ADC, DMA, interrupções, OLED, etc.

        while (true)
        {
                task_1_read_temperature();
                task_5_alert_neopixel();
                task_3_thermal_trend();
                task_2_show_oled();
                task_4_update_neopixel_matrix();

                // --- Cálculo dos tempos de execução ---
                int64_t tempo1_us = absolute_time_diff_us(ini_tarefa1, fim_tarefa1);
                int64_t tempo2_us = absolute_time_diff_us(ini_tarefa2, fim_tarefa2);
                int64_t tempo3_us = absolute_time_diff_us(ini_tarefa3, fim_tarefa3);
                int64_t tempo4_us = absolute_time_diff_us(ini_tarefa4, fim_tarefa4);

                // --- Exibição no terminal ---
                printf("Temperatura: %.2f °C | T1: %.7fs | T2: %.7fs | T3: %.7fs | T4: %.7fs | Tendência: %s\n",
                       media,
                       tempo1_us / 1e6,
                       tempo2_us / 1e6,
                       tempo3_us / 1e6,
                       tempo4_us / 1e6,
                       tendencia_para_texto(t));
        }

        return 0;
}

/*******************************/
void task_1_read_temperature()
{
        // --- Tarefa 1: Leitura de temperatura via DMA ---
        ini_tarefa1 = get_absolute_time();
        media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL);
        fim_tarefa1 = get_absolute_time();
}
/*******************************/
void task_3_thermal_trend()
{
        // --- Tarefa 3: Análise da tendência térmica ---
        ini_tarefa3 = get_absolute_time();
        t = tarefa3_analisa_tendencia(media);
        fim_tarefa3 = get_absolute_time();
}
/*******************************/
void task_2_show_oled()
{
        // --- Tarefa 2: Exibição no OLED ---
        ini_tarefa2 = get_absolute_time();
        tarefa2_exibir_oled(media, t);
        fim_tarefa2 = get_absolute_time();
}
/*******************************/
void task_4_update_neopixel_matrix()
{
        // --- Tarefa 4: Cor da matriz NeoPixel por tendência ---
        absolute_time_t ini_tarefa4 = get_absolute_time();
        tarefa4_matriz_cor_por_tendencia(t);
        absolute_time_t fim_tarefa4 = get_absolute_time();
}
void task_5_alert_neopixel()
{
        // --- Tarefa 5: Extra ---
        while (media < 1)
        {
                npSetAll(COR_BRANCA);
                npWrite();
                sleep_ms(1000);
                npClear();
                npWrite();
                sleep_ms(1000);
        }
}