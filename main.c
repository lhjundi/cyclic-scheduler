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
void show_duration_tasks_execution();

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
                show_duration_tasks_execution();
        }

        return 0;
}

void show_duration_tasks_execution()
{
        // --- Cálculo dos tempos de execução ---
        int64_t tempo1_us = absolute_time_diff_us(ini_tarefa1, fim_tarefa1);
        int64_t tempo2_us = absolute_time_diff_us(ini_tarefa2, fim_tarefa2);
        int64_t tempo3_us = absolute_time_diff_us(ini_tarefa3, fim_tarefa3);
        int64_t tempo4_us = absolute_time_diff_us(ini_tarefa4, fim_tarefa4);

        // --- Exibição no terminal ---
        printf("Temperatura: %.2f °C | T1: %.6fs | T2: %.6fs | T3: %.6fs | T4: %.6fs | Tendência: %s\n",
               media,
               tempo1_us / 1e6,
               tempo2_us / 1e6,
               tempo3_us / 1e6,
               tempo4_us / 1e6,
               tendencia_para_texto(t));
}

/**
 * @brief Reads the temperature, calculates the average, and records timing.
 *
 * This function marks the start time, obtains the average temperature reading
 * using the configuration and DMA channel, and then marks the end time.
 *
 * Globals used:
 *  - ini_tarefa1: Stores the start timestamp of the task.
 *  - fim_tarefa1: Stores the end timestamp of the task.
 *  - media: Stores the calculated average temperature.
 *  - cfg_temp: Configuration structure for temperature reading.
 *  - DMA_TEMP_CHANNEL: DMA channel used for temperature sensor.
 *
 * Functions called:
 *  - get_absolute_time(): Returns the current absolute time.
 *  - tarefa1_obter_media_temp(): Calculates the average temperature.
 */
void task_1_read_temperature()
{
        ini_tarefa1 = get_absolute_time();
        media = tarefa1_obter_media_temp(&cfg_temp, DMA_TEMP_CHANNEL);
        fim_tarefa1 = get_absolute_time();
}


/**
 * @brief Executes the thermal trend analysis task.
 *
 * This function records the start and end times of the thermal trend analysis task.
 * It calls `tarefa3_analisa_tendencia()` with the current average value (`media`)
 * to analyze the thermal trend and stores the result in `t`.
 *
 * Global variables used:
 * - ini_tarefa3: Stores the start time of the task.
 * - fim_tarefa3: Stores the end time of the task.
 * - media: The average value used for trend analysis.
 * - t: Variable to store the result of the trend analysis.
 */
void task_3_thermal_trend()
{
        ini_tarefa3 = get_absolute_time();
        t = tarefa3_analisa_tendencia(media);
        fim_tarefa3 = get_absolute_time();
}

/**
 * @brief Executes the OLED display task.
 *
 * This function records the start time, calls the function to display data on the OLED
 * (using the variables 'media' and 't'), and then records the end time.
 *
 * It is assumed that 'ini_tarefa2' and 'fim_tarefa2' are used for timing measurements,
 * and that 'media' and 't' are available in the current scope.
 */
void task_2_show_oled()
{
        ini_tarefa2 = get_absolute_time();
        tarefa2_exibir_oled(media, t);
        fim_tarefa2 = get_absolute_time();
}

/**
 * @brief Updates the NeoPixel matrix based on the current trend.
 *
 * This function records the start and end times of the update operation for profiling or timing purposes.
 * It calls `tarefa4_matriz_cor_por_tendencia(t)` to update the matrix colors according to the trend.
 *
 * Note: The variable `t` should be defined and initialized appropriately before calling this function.
 */
void task_4_update_neopixel_matrix()
{
        ini_tarefa4 = get_absolute_time();
        tarefa4_matriz_cor_por_tendencia(t);
        fim_tarefa4 = get_absolute_time();
}

/**
 * @brief Controls the NeoPixel alert based on the value of 'media'.
 *
 * If 'media' is less than 1, all NeoPixels are set to the color defined by COR_BRANCA and updated.
 * Otherwise, all NeoPixels are cleared and updated.
 *
 * Assumes that 'media' is a global or accessible variable, and that npSetAll, npWrite, npClear,
 * and COR_BRANCA are defined elsewhere in the codebase.
 */
void task_5_alert_neopixel()
{
        if(media < 1){
                npSetAll(COR_BRANCA);
                npWrite();
        }
        else{
                npClear();
                npWrite();
        }
}