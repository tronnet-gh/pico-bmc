#include "lwip/apps/httpd.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"
#include "ssi.h"
#include "cgi.h"
#include "secret.h"

int main() {
	stdio_init_all();

	if (cyw43_arch_init()) {
		printf("Wi-Fi init failed\n");
		return -1;
	}
	printf("Wi-Fi init succeeded\n");

	cyw43_arch_enable_sta_mode();

	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)){
		printf("Wi-Fi failed to connect\n");
		return -1;
	}
	printf("Wi-Fi connected\n");
	
	// init httpd
	httpd_init();
	printf("HTTP Server initialized at %s on port %d\n", ip4addr_ntoa(netif_ip4_addr(netif_list)), HTTPD_SERVER_PORT);

	// enable ssi
	ssi_init();
	printf("SSI Handler initialized\n");

	// enable cgi
	cgi_init();
	printf("CGI Handler initialised\n");

	// init bmc handler
	bmc_handler_init();
	printf("BMC handler initialized\n");

	// set LED to on to indicate it has connected and initialized
	cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

	while(1);
}