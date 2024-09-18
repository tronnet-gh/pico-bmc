#ifndef SERIAL_HANDLER_H
#define SERIAL_HANDLER_H

#define BUF_SIZE 1024

char * input_buffer_0;
char * input_buffer_1;
char * input_buffers [2] = {NULL, NULL};
bool active_write_input_buffer;
int write_offset;
semaphore_t sem_line;

int append_char (char c) {
	if (write_offset < BUF_SIZE - 1) {
		input_buffers[active_write_input_buffer][write_offset] = c;
		write_offset++;
		return true;
	}
	else {
		DEBUG_printf("[SER ] [ERR] write_offset overflow\n");
		return false;
	}	
}

int delete_char () {
	if (write_offset > 0) {
		write_offset--;
		input_buffers[active_write_input_buffer][write_offset] = 0;
		return true;
	}
	else {
		DEBUG_printf("[SER ] [ERR] write_offset underflow\n");
		return false;
	}
}

void reset_input () {
	write_offset = 0;
}

void serial_get_line (char * buf, int len) {
	sem_acquire_blocking(&sem_line);
	memcpy(buf, input_buffers[!active_write_input_buffer], BUF_SIZE > len ? BUF_SIZE : len);
}

// interrupt code adapted from: https://github.com/raspberrypi/pico-examples/blob/eca13acf57916a0bd5961028314006983894fc84/pico_w/wifi/iperf/picow_iperf.c
// and fixed using the suggestion from: https://github.com/raspberrypi/pico-examples/issues/474

// final interrupt callback which consumes the keypress
// when it recieves a character, echo back to the console 
void key_pressed_worker_func(async_context_t * context, async_when_pending_worker_t * worker) {
	int key;
	while ((key = getchar_timeout_us(0)) > 0) {
		if (key == 0x08 || key == 0x7F) { // backspace & delete
			if (delete_char()) {
				printf("\b \b");
			}
		}
		else if (key == '\r' || key == '\n') { // return
			printf("\n");
			append_char(0); // ensure there is a null terminator
			active_write_input_buffer = !active_write_input_buffer; // flip the active buffer
			reset_input(); // reset write pointer to 0
			sem_release(&sem_line); // release sem to allow serial_get_line to resume
		}
		else if (32 <= key && key <= 126) { // printable characters
			printf("%c", key);
			append_char(key);
		}
		else if (key == 27) { // escape & escape codes
			while (getchar_timeout_us(0) > 0) {
				// TODO handle escape codes
			}
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
	async_context_set_work_pending((async_context_t *) param, &key_pressed_worker);
}

// init serial handler
void serial_handler_init () {
	input_buffer_0 = malloc(BUF_SIZE * sizeof(char));
	input_buffer_1 = malloc(BUF_SIZE * sizeof(char));
	input_buffers[0] = input_buffer_0;
	input_buffers[1] = input_buffer_1;
	sem_init(&sem_line, 0, 1);
	async_context_add_when_pending_worker(cyw43_arch_async_context(), &key_pressed_worker);
	stdio_set_chars_available_callback(key_pressed_func, cyw43_arch_async_context());
}

void serial_handler_deinit () {
	free(input_buffer_0);
	free(input_buffer_1);
	input_buffer_0 = NULL;
	input_buffer_1 = NULL;
	input_buffers[0] = NULL;
	input_buffers[1] = NULL;
}

#endif