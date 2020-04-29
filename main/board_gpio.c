#include "driver/gpio.h"

#define LED_GPIO 33
#define POWER_GPIO 4
#define BUTTON_GPIO 15

#define LED_ON_STATE 0
#define LEF_OFF_STATE 1

void gpio_init()
{
	gpio_pad_select_gpio(LED_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
	
	gpio_pad_select_gpio(POWER_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(POWER_GPIO, GPIO_MODE_OUTPUT);
	
	gpio_pad_select_gpio(BUTTON_GPIO);
    /* Set the GPIO as input */
    gpio_set_direction(BUTTON_GPIO, GPIO_MODE_INPUT);

	
}

void led_on()
{
	gpio_set_level(LED_GPIO, LED_ON_STATE);
}

void led_off()
{
	gpio_set_level(LED_GPIO, LEF_OFF_STATE);
}

void led_set(uint32_t level)
{
	if (level==0)
		gpio_set_level(LED_GPIO, LEF_OFF_STATE);
	else gpio_set_level(LED_GPIO, LED_ON_STATE);
}

void power_on()
{
	gpio_set_level(POWER_GPIO, 1);
}

void power_off()
{
	gpio_set_level(POWER_GPIO, 0);
}

void power_set(uint32_t level)
{
	gpio_set_level(POWER_GPIO, level);
}

uint32_t button_get()
{
	return gpio_get_level(BUTTON_GPIO);
}