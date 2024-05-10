#ifndef NETWORK_H
#define NETWORK_H

void set_host_name(const char * hostname) {
	cyw43_arch_lwip_begin();
	struct netif * n = &cyw43_state.netif[CYW43_ITF_STA];
	netif_set_hostname(n, hostname);
	netif_set_up(n);
	cyw43_arch_lwip_end();
}

int network_init (char * hostname, char * wifi_ssid, char * wifi_pass) {
    set_host_name(hostname);
    DEBUG_printf("[INIT] [OK ] Set hostname to %s\n", BMC_HOSTNAME);

	if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 30000)){
		DEBUG_printf("[INIT] [ERR] Wi-Fi failed to connect\n");
		return -1;
	}
	else {
		DEBUG_printf("[INIT] [OK ] Wi-Fi connected successfully\n");
		return 0;
	}
}

int network_deinit () {}

#endif