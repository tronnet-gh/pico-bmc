#ifndef HANDLERS_H
#define HANDLERS_H

#include "pico/stdlib.h"

#define PW_SW_DELAY_MS 100
#define STATE_UPDATE_REPEAT_DELAY_MS 100

bool current_state = false;
struct repeating_timer * state_update_timer = NULL; 

int64_t pw_sw_on_async (alarm_id_t id, void * user_data) {
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
	return 0; // do not reschedule alarm
}

int64_t pw_sw_off_async (alarm_id_t id, void * user_data) {
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
	return 0; // do not reschedule alarm
}

bool update_current_state_async (repeating_timer_t * rt) {
	return true; // continue repeating alarm
}

bool bmc_power_handler (bool requested_state) {
	if (requested_state != current_state) {
		add_alarm_in_ms(0, pw_sw_on_async, NULL, true);
		add_alarm_in_ms(PW_SW_DELAY_MS, pw_sw_off_async, NULL, true);
	}
}

void bmc_handler_init () {
	state_update_timer = malloc(sizeof(struct repeating_timer));
	add_repeating_timer_ms(STATE_UPDATE_REPEAT_DELAY_MS, update_current_state_async, NULL, state_update_timer);
}

void bmc_handler_deinit () {
	cancel_repeating_timer(state_update_timer);
	free(state_update_timer);
	state_update_timer = NULL;
}

#endif