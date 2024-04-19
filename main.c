#define TCP_PORT 80
#define DEBUG_printf printf

#include "pico_lib.h"
#include "bmc_handler.h"
#include "http_serv.h"
#include "secret.h"

void set_host_name(const char * hostname) {
    cyw43_arch_lwip_begin();
    struct netif *n = &cyw43_state.netif[CYW43_ITF_STA];
    netif_set_hostname(n, hostname);
    netif_set_up(n);
    cyw43_arch_lwip_end();
}

int main() {
	stdio_init_all();

	if (cyw43_arch_init()) {
		DEBUG_printf("[INIT] [ERR] Failed to initialise cyw43\n");
		return 1;
	}

	cyw43_arch_enable_sta_mode();
	set_host_name(BMC_HOSTNAME);
	DEBUG_printf("[INIT] [OK ] Set hostname to %s\n", BMC_HOSTNAME);

	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)){
		DEBUG_printf("[INIT] [ERR] Wi-Fi failed to connect\n");
		return -1;
	}
	DEBUG_printf("[INIT] [OK ] Wi-Fi connected successfully\n");

	http_serv_init();

	// init bmc handler
	bmc_handler_init();

	// set LED to on to indicate it has connected and initialized
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

	while (!http_serv_state->complete) {
		sleep_ms(1000);
	}

	bmc_handler_deinit();
	http_serv_deinit();
	cyw43_arch_deinit();
	return 0;
}