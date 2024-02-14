#ifndef CGI_H
#define CGI_H

#include "lwip/apps/httpd.h"
#include "pico/cyw43_arch.h"
#include "handlers.h"

char parseRequestedState (char * requested_state) {
	
}

const char * cgi_power_handler (int iIndex, int iNumParams, char * pcParam [], char * pcValue []) {
	// Check if an request for power has been made (/power?requested_state=x)
	if (strcmp(pcParam[0] , "requested_state") == 0){
		if (strcmp(pcValue[0], "0") == 0) {
			bmc_power_handler(false);
			return "/power.ssi";
		}
		else if (strcmp(pcValue[0], "1") == 0) {
			bmc_power_handler(true);
			return "/power.ssi";
		}
		else {
			return "/error_requested_state_invalid.ssi";
		}
	}
	// Send the index page back to the user
	else {
		return "/error_missing_requested_state.ssi";
	}
}

const char * cgi_status_handler (int iIndex, int iNumParams, char *pcParam[], char *pcValue[]) {
	return "/status.ssi";
}

static const tCGI cgi_handlers[] = {
	{
		"/power", cgi_power_handler
	},
	{
		"/status", cgi_status_handler
	}
};

void cgi_init(void) {
	http_set_cgi_handlers(cgi_handlers, 2);
}

#endif