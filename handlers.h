#ifndef HANDLERS_H
#define HANDLERS_H

#include "pico/stdlib.h"

#define PW_SWITCH_PIN 0
#define PW_SWITCH_DELAY_MS 100
#define PW_STATE_PIN 1
#define PW_STATE_UPDATE_REPEAT_DELAY_MS 100

bool current_state = false;
struct repeating_timer * state_update_timer = NULL; 

// handler fn to set the power switch pin to an active state
int64_t pw_sw_on_async (alarm_id_t id, void * user_data) {
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
	gpio_put(PW_SWITCH_PIN, 1);
	return 0; // do not reschedule alarm
}

// handler fn to set the power switch pin to an inactive state
int64_t pw_sw_off_async (alarm_id_t id, void * user_data) {
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
	gpio_put(PW_SWITCH_PIN, 0);
	return 0; // do not reschedule alarm
}

// hander fn to read from the power state
bool update_current_state_async (repeating_timer_t * rt) {
	current_state = gpio_get(PW_STATE_PIN);
	return true; // continue repeating alarm
}

// handler fn called to attempt to set the power state to the requested state
bool bmc_power_handler (bool requested_state) {
	if (requested_state != current_state) {
		add_alarm_in_ms(0, pw_sw_on_async, NULL, true);
		add_alarm_in_ms(PW_SWITCH_DELAY_MS, pw_sw_off_async, NULL, true);
	}
}

void bmc_handler_init () {
	// init power switch pin as output
	gpio_init(PW_SWITCH_PIN);
	gpio_set_dir(PW_SWITCH_DELAY_MS, GPIO_OUT);

	// init power state pin as input
	gpio_init(PW_STATE_PIN);
	gpio_set_dir(PW_STATE_PIN, GPIO_IN);

	// set repeating timer for power state update
	state_update_timer = malloc(sizeof(struct repeating_timer));
	add_repeating_timer_ms(PW_STATE_UPDATE_REPEAT_DELAY_MS, update_current_state_async, NULL, state_update_timer);
}

void bmc_handler_deinit () {
	cancel_repeating_timer(state_update_timer);
	free(state_update_timer);
	state_update_timer = NULL;
}

#endif