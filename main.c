#define TCP_PORT 80
#define DEBUG_printf printf

#include "pico_lib.h"
#include "handlers.h"
#include "http_serv.h"
#include "secret.h"

int main() {
    stdio_init_all();

    if (cyw43_arch_init()) {
        DEBUG_printf("[INIT] [ERR] Failed to initialise cyw43\n");
        return 1;
    }

	cyw43_arch_enable_sta_mode();

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

    while(true);

	http_serv_deinit();
    cyw43_arch_deinit();
    return 0;
}