#ifndef BMC_HANDLER_H
#define BMC_HANDLER_H

#define PW_SWITCH_PIN 0
#define PW_SWITCH_DELAY_MS 100
#define PW_SWITCH_INV 0
#define PW_STATE_PIN 1
#define PW_STATE_UPDATE_REPEAT_DELAY_MS 100
#define PW_STATE_INV 1

typedef struct CURRENT_STATE_T_ {
	float voltage;
	float tempC;
	bool power_state;
} CURRENT_STATE_T;

struct repeating_timer * state_update_timer = NULL;

CURRENT_STATE_T current_state;

// handler fn to set the power switch pin to an active state
int64_t pw_sw_on_async (alarm_id_t id, void * user_data) {
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
	gpio_put(PW_SWITCH_PIN, 1 ^ PW_SWITCH_INV);
	return 0; // do not reschedule alarm
}

// handler fn to set the power switch pin to an inactive state
int64_t pw_sw_off_async (alarm_id_t id, void * user_data) {
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
	gpio_put(PW_SWITCH_PIN, 0 ^ PW_SWITCH_INV);
	return 0; // do not reschedule alarm
}

// hander fn to read from the power state
bool update_current_state_async (repeating_timer_t * rt) {
	current_state.voltage = adc_read() * 3.3f / (1 << 12);
	current_state.tempC = 27.0f - (current_state.voltage - 0.706f) / 0.001721f;
	current_state.power_state = gpio_get(PW_STATE_PIN) ^ PW_STATE_INV;
	return true; // continue repeating alarm
}

// handler fn called to attempt to set the power state to the requested state
void bmc_power_handler (bool requested_power_state) {
	if (requested_power_state != current_state.power_state) {
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

	// init adc input
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);

	// set repeating timer for power state update
	state_update_timer = malloc(sizeof(struct repeating_timer));
	add_repeating_timer_ms(PW_STATE_UPDATE_REPEAT_DELAY_MS, update_current_state_async, NULL, state_update_timer);

	DEBUG_printf("[BMC ] [OK ] BMC initialized successfully\n");
}

void bmc_handler_deinit () {
	cancel_repeating_timer(state_update_timer);
	free(state_update_timer);
	state_update_timer = NULL;
}

#endif