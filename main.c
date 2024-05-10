#ifndef NDEBUG
#define DEBUG_printf printf
#else
#define DEBUG_printf
#endif

#include "secret.h"
#include "pico_lib.h"
#include "network.h"
#include "bmc_handler.h"
#include "http_serv.h"
#include "serial_handler.h"

int main() {
	stdio_init_all();

	if (cyw43_arch_init()) {
		DEBUG_printf("[INIT] [ERR] Failed to initialise cyw43\n");
		return 1;
	}
	cyw43_arch_enable_sta_mode();

	network_init(BMC_HOSTNAME, WIFI_SSID, WIFI_PASS);
	http_serv_init();
	bmc_handler_init();
	serial_handler_init();

	// set LED to on to indicate it has connected and initialized
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

	char command [BUF_SIZE];
	while (!http_serv_state->complete) {
		printf("> ");
		serial_get_line(command, BUF_SIZE);

		if (strcmp(command, "help") == 0) {
			printf("PICO BMC Shell\n\nhelp\tshow this help message\ninfo\tshow network name, hostname, ip address, and http server port\nclear\tclears the screen\n");
		}
		else if (strcmp(command, "info") == 0) {
			printf("Network: %s\nHostname: %s\nAddress: %s\nHTTP Port: %i\n", WIFI_SSID, BMC_HOSTNAME, ip_ntoa(netif_ip4_addr(&cyw43_state.netif[CYW43_ITF_STA])), HTTP_PORT);
		}
		else if (strcmp(command, "clear") == 0) {
			printf("\033[2J\033[H");
		}
		else if (strcmp(command, "") == 0) {
			continue;
		}
		else {
			printf("Unknown command: %s\n", command);
		}
	}

	serial_handler_deinit();
	bmc_handler_deinit();
	http_serv_deinit();
	network_deinit();
	cyw43_arch_deinit();
	return 0;
}