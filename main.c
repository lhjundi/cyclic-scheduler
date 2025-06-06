/**
 * @file main.c
 * @brief Cyclic scheduler for task management on Raspberry Pi Pico.
 *
 * This file implements a cyclic scheduler that coordinates the execution of multiple tasks
 * (temperature reading, OLED display update, thermal trend analysis, NeoPixel matrix update, and alert)
 * using state flags and periodic timers. Each task is triggered by a repeating timer callback that sets
 * a corresponding flag. The main control loop checks these flags in a fixed order and executes one task
 * per iteration, updating the state to transition to the next task in the cycle.
 *
 * Key Features:
 * - Uses Pico SDK for hardware abstraction and timing.
 * - Modular task functions for temperature acquisition, display, trend analysis, and NeoPixel control.
 * - Timing and profiling of each task's execution duration.
 * - State management for cyclic scheduling using boolean flags.
 * - Integration with DMA, ADC, and external display/LED drivers.
 *
 * Global Variables:
 * - Flags for each task's readiness (can_read_temp, can_alert_neopixel, etc.).
 * - Timing variables for profiling (ini_tarefaX, fim_tarefaX).
 * - Shared data (media for temperature, t for trend).
 *
 * Functions:
 * - control_states(): Main scheduler, executes tasks based on flags.
 * - update_states(): Advances state flags for cyclic scheduling.
 * - show_duration_tasks_execution(): Prints timing and temperature/trend info.
 * - task_1_read_temperature(): Reads and averages temperature.
 * - task_2_show_oled(): Updates OLED display.
 * - task_3_thermal_trend(): Analyzes temperature trend.
 * - task_4_update_neopixel_matrix(): Updates NeoPixel matrix based on trend.
 * - task_5_alert_neopixel(): Controls NeoPixel alert based on temperature.
 * - Timer callbacks to set task flags.
 *
 * Usage:
 * - Initialize hardware and timers in main().
 * - Scheduler loop in main() calls control_states() continuously.
 *
 * Dependencies:
 * - Pico SDK (hardware, stdlib, timer, watchdog)
 * - External modules: setup.h, tarefa1_temp.h, tarefa2_display.h, tarefa3_tendencia.h,
 *   tarefa4_controla_neopixel.h, neopixel_driver.h, testes_cores.h
 * 
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
#include "hardware/timer.h"

volatile float media;
volatile tendencia_t t;
volatile absolute_time_t ini_tarefa1, fim_tarefa1, ini_tarefa2, fim_tarefa2, ini_tarefa3, fim_tarefa3, ini_tarefa4, fim_tarefa4;
bool can_read_temp = true;
bool can_alert_neopixel = false;
bool can_thermal_trend = false;
bool can_show_oled = false;
bool can_update_neopixel_matrix = false;
bool can_show_durations = false;

void task_3_thermal_trend();
void task_5_alert_neopixel();
void task_4_update_neopixel_matrix();
void task_2_show_oled();
void task_1_read_temperature();
void show_duration_tasks_execution();
void update_states(bool *this_state, bool *next_state);
void control_states();

/**
 * @brief Callback function for a repeating timer.
 *
 * This function is called each time the repeating timer elapses.
 * It sets the global flag `can_read_temp` to true, indicating that
 * a temperature reading can be performed. The function returns true
 * to keep the timer active and continue calling this callback.
 *
 * @param t Pointer to the repeating_timer_t structure associated with the timer.
 * @return true to keep the timer running; false to stop the timer.
 */
bool repeating_timer_callback(repeating_timer_t *t)
{
        can_read_temp = true;
        return true;
}

/**
 * @brief Callback function for task 5, triggered by a repeating timer.
 *
 * This function is called each time the associated repeating timer expires.
 * It sets the global flag `can_alert_neopixel` to true, indicating that
 * the NeoPixel alert can be triggered. The function returns true to keep
 * the timer active and continue calling this callback on subsequent intervals.
 *
 * @param t Pointer to the repeating_timer_t structure associated with the timer event.
 * @return true to keep the timer running and continue invoking the callback.
 */
bool task5_callback(repeating_timer_t *t)
{
        can_alert_neopixel = true;
        return true;
}

/**
 * @brief Callback function for task 3, triggered by a repeating timer.
 *
 * This function is called each time the associated repeating timer expires.
 * It sets the global variable `can_thermal_trend` to true, indicating that
 * a thermal trend operation can proceed. The function returns true to
 * continue the timer's periodic execution.
 *
 * @param t Pointer to the repeating_timer_t structure representing the timer event.
 * @return true to keep the timer running for subsequent callbacks.
 */
bool task3_callback(repeating_timer_t *t)
{
        can_thermal_trend = true;
        return true;
}

/**
 * @brief Callback function for task 2, triggered by a repeating timer.
 *
 * This function is called each time the associated repeating timer expires.
 * It sets the global variable `can_show_oled` to true, indicating that the OLED display
 * can be updated or shown. The function returns true to keep the timer repeating.
 *
 * @param t Pointer to the repeating_timer_t structure associated with this callback.
 * @return true to continue repeating the timer, false to stop.
 */
bool task2_callback(repeating_timer_t *t)
{
        can_show_oled = true;
        return true;
}

/**
 * @brief Callback function for task 4, triggered by a repeating timer.
 *
 * This function sets the flag `can_update_neopixel_matrix` to true,
 * indicating that the NeoPixel matrix can be updated. It returns true
 * to keep the repeating timer active.
 *
 * @param t Pointer to the repeating_timer_t structure associated with the timer event.
 * @return true to continue repeating the timer, false to stop.
 */
bool task4_callback(repeating_timer_t *t)
{
        can_update_neopixel_matrix = true;
        return true;
}

/**
 * @brief Controls the execution of various system tasks based on their state flags.
 *
 * This function checks a series of boolean flags, each representing whether a specific task should be executed.
 * For each flag that is set to true, the corresponding task function is called, and the state is updated to transition
 * to the next task in the sequence. Only one task is executed per function call, based on the order of checks.
 *
 * @param read_temp Pointer to a boolean flag indicating if the temperature reading task should run.
 * @param alert_neopixel Pointer to a boolean flag indicating if the Neopixel alert task should run.
 * @param thermal_trend Pointer to a boolean flag indicating if the thermal trend analysis task should run.
 * @param show_oled Pointer to a boolean flag indicating if the OLED display update task should run.
 * @param update_neopixel_matrix Pointer to a boolean flag indicating if the Neopixel matrix update task should run.
 * @param show_durations Pointer to a boolean flag indicating if the task durations display should run.
 *
 * @note The function executes only one task per call, based on the first flag found to be true in the order listed.
 *       After executing a task, it updates the states to prepare for the next task in the cycle.
 */
void control_states()
{
        if (can_read_temp)
        {
                task_1_read_temperature();
                update_states(&can_read_temp, &can_alert_neopixel);
                return;
        }
        if (can_alert_neopixel)
        {
                task_5_alert_neopixel();
                update_states(&can_alert_neopixel, &can_thermal_trend);
                return;
        }
        if (can_thermal_trend)
        {
                task_3_thermal_trend();
                printf("Tendência: %s\n", tendencia_para_texto(t));
                update_states(&can_thermal_trend, &can_show_oled);
                return;
        }
        if (can_show_oled)
        {
                task_2_show_oled();
                update_states(&can_show_oled, &can_update_neopixel_matrix);
                return;
        }
        if (can_update_neopixel_matrix)
        {
                can_update_neopixel_matrix = false; // Reset the flag to avoid re-triggering
                task_4_update_neopixel_matrix();
                show_duration_tasks_execution();
                return;
        }
}

/**
 * @brief Updates the state variables for a cyclic scheduler.
 *
 * This function copies the value of the current state (`this_state`) to the next state (`next_state`),
 * and then resets the current state to false. It is typically used to advance state flags in a cyclic
 * or time-triggered scheduler.
 *
 * @param this_state Pointer to the current state boolean variable. Will be set to false after the call.
 * @param next_state Pointer to the next state boolean variable. Will be updated with the value of `this_state`.
 */
void update_states(bool *this_state, bool *next_state)
{
        (*next_state) = (*this_state);
        (*this_state) = false;
}

/**
 * @brief Displays the execution duration of four tasks along with temperature information and trend.
 *
 * This function calculates the execution time (in microseconds) for four tasks using their respective
 * start and end timestamps. It then prints the average temperature, the execution time of each task
 * (converted to seconds with microsecond precision), and the current trend as a formatted message.
 *
 * Assumes the following external/global variables and functions are available:
 * - ini_tarefa1, fim_tarefa1, ini_tarefa2, fim_tarefa2, ini_tarefa3, fim_tarefa3, ini_tarefa4, fim_tarefa4:
 *   Timestamps marking the start and end of each task.
 * - absolute_time_diff_us(): Function to compute the time difference in microseconds.
 * - media: The average temperature value.
 * - tendencia_para_texto(): Function to convert the trend indicator to a human-readable string.
 * - t: The current trend indicator.
 */
void show_duration_tasks_execution()
{
        int64_t tempo1_us = absolute_time_diff_us(ini_tarefa1, fim_tarefa1);
        int64_t tempo2_us = absolute_time_diff_us(ini_tarefa2, fim_tarefa2);
        int64_t tempo3_us = absolute_time_diff_us(ini_tarefa3, fim_tarefa3);
        int64_t tempo4_us = absolute_time_diff_us(ini_tarefa4, fim_tarefa4);

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
        can_read_temp = false; // Reset the flag to avoid re-triggering
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
        printf("Exibindo no OLED: %.2f °C | Tendência: %s\n", media, tendencia_para_texto(t));
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
        printf("Atualizando matriz NeoPixel com a tendência: %s\n", tendencia_para_texto(t));
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
        if (media < 1)
        {
                npSetAll(COR_BRANCA);
                npWrite();
        }
        else
        {
                npClear();
                npWrite();
        }
        printf("Task 5! \n");
}
/**
 * @brief Main entry point of the cyclic scheduler application.
 *
 * This function initializes and starts multiple repeating timers with different
 * intervals, each associated with a specific callback function to handle periodic tasks.
 * The timers are used to schedule tasks at regular intervals, enabling cyclic execution.
 *
 * The `setup()` function is called to perform necessary initializations such as
 * configuring the ADC, DMA, interrupts, OLED display, and other peripherals.
 *
 * The main loop continuously calls `control_states()`, which is responsible for
 * managing the system's state based on the flags set by the timer callbacks.
 *
 * @return int Returns 0 upon successful execution (though this point is never reached).
 */

int main()
{
        // adcionar funções de callback que alteram o estado das flags

        static repeating_timer_t timer1, timer2, timer3, timer4, timer5;
        add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &timer1);
        add_repeating_timer_ms(1200, task5_callback, NULL, &timer5);
        add_repeating_timer_ms(1250, task3_callback, NULL, &timer3);
        add_repeating_timer_ms(1300, task2_callback, NULL, &timer2);
        add_repeating_timer_ms(1350, task4_callback, NULL, &timer4);

        setup(); // Inicializações: ADC, DMA, interrupções, OLED, etc.

        while (true)
        {
                control_states();
        }

        return 0;
}