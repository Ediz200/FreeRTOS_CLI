/*
 * led_effect.c
 *
 *  Created on: Jul 4, 2025
 *      Author: edizt
 */

#include "main.h"

void led_control(int n)
{
    const LedPin leds[4] = {
        {GPIOC, GREEN_Pin},
        {GPIOC, RED_Pin},
        {GPIOC, ORANGE_Pin},
        {GPIOD, BLUE_Pin}
    };

    for (int i = 0; i < 4; ++i)
    {
        HAL_GPIO_WritePin(leds[i].port, leds[i].pin, (i == n) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void turn_off_all_leds(void)
{
	HAL_GPIO_WritePin(GPIOC, GREEN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, RED_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, ORANGE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, BLUE_Pin, GPIO_PIN_SET);
}

void turn_on_all_leds(void)
{
	HAL_GPIO_WritePin(GPIOC, GREEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, RED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, ORANGE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, BLUE_Pin, GPIO_PIN_RESET);
}

void toggle_even_leds(void)
{
	HAL_GPIO_WritePin(GPIOC, GREEN_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, RED_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, ORANGE_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOD, BLUE_Pin, GPIO_PIN_SET);
}

void toggle_odd_leds(void)
{
	HAL_GPIO_WritePin(GPIOC, GREEN_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, RED_Pin, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOC, ORANGE_Pin, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, BLUE_Pin, GPIO_PIN_RESET);
}

void led_effect_stop(void)
{
	for(int i=0; i<4; i++){
		xTimerStop(handle_led_timer[i], portMAX_DELAY);
	}
}

void led_effect(int n)
{
	led_effect_stop();
	xTimerStart(handle_led_timer[n-1], portMAX_DELAY);
}

void led_effect1(void)
{
	static int flag = 1;
	(flag ^= 1) ? turn_off_all_leds() : turn_on_all_leds();
}

void led_effect2(void)
{
	static int flag = 1;
	(flag ^= 1) ? toggle_even_leds() : toggle_odd_leds();
}

void led_effect3(void)
{
	static int i = 0;
	led_control((i++ % 4));
}

void led_effect4(void)
{
    static int i = 3;

    led_control(i);
    i--;

    if (i < 0)
        i = 3;
}



