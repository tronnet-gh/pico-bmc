#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#define INPUT_BUFFER_SIZE 1024

char * input_buffer;
int write_offset;

bool inc_write_offset () {
	if (write_offset < INPUT_BUFFER_SIZE - 1) {
		write_offset++;
		return true;
	}
	else {
		DEBUG_printf("[SER ] [ERR] write_offset overflow\n");
		return false;
	}	
}

bool dec_write_offset () {
	if (write_offset > 0) {
		write_offset--;
		return true;
	}
	else {
		DEBUG_printf("[SER ] [ERR] write_offset underflow\n");
		return false;
	}
}

void rst_write_offset () {
	write_offset = 0;
}

void handle_return () {
	if (strcmp(input_buffer, "") == 0) {
		return;
	}
	else if (strcmp(input_buffer, "info") == 0) {
		printf("Network: %s\nHostname: %s\nAddress: %s\nHTTP Port: %i\n", WIFI_SSID, BMC_HOSTNAME, ip_ntoa(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA])), HTTP_PORT);
	}
	else {
		printf("Unknown command: %s\n", input_buffer);
	}
}

// interrupt code adapted from: https://github.com/raspberrypi/pico-examples/blob/eca13acf57916a0bd5961028314006983894fc84/pico_w/wifi/iperf/picow_iperf.c
// and fixed using the suggestion from: https://github.com/raspberrypi/pico-examples/issues/474

// final interrupt callback which consumes the keypress
// when it recieves a character, echo back to the console 
void key_pressed_worker_func(async_context_t * context, async_when_pending_worker_t * worker) {
	int key;
	while ((key = getchar_timeout_us(0)) > 0) {
		if (key == 0x08 || key == 0x7F) { // backspace & delete
			if (dec_write_offset()) {
				printf("\b \b");
				input_buffer[write_offset] = 0;
			}
		}
		else if (key == '\r' || key == '\n') { // return
			printf("\n");
			input_buffer[write_offset] = 0;
			handle_return();
			rst_write_offset();
		}
		else if (32 <= key && key <= 126) { // printable characters
			printf("%c", key);
			input_buffer[write_offset] = key;
			inc_write_offset();
		}
		else if (key == 27) { // escape & escape codes
			while (getchar_timeout_us(0) > 0) {}
			break;
		}
		else {	// other unhandled key
			DEBUG_printf("[SER ] [ERR] recieved unhandled key [%i]\n", key);
		}
	}
}

static async_when_pending_worker_t key_pressed_worker = {
	.do_work = key_pressed_worker_func
};

// because of stdio_set_chars_available_callback implementation, stdlib mutex is locked here so schedule another callback
void key_pressed_func(void * param) {
	async_context_set_work_pending((async_context_t *)param, &key_pressed_worker);
}

// init serial handler
void serial_handler_init () {
	input_buffer = malloc(INPUT_BUFFER_SIZE * sizeof(char));
	async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
	stdio_set_chars_available_callback(key_pressed_func, cyw43_arch_async_context());
}

void serial_handler_deinit () {
	free(input_buffer);
	input_buffer = NULL;
}

#endif