#ifndef SSI_H
#define SSI_H

#include "lwip/apps/httpd.h"
#include "handlers.h"

const char * ssi_tags[] = {"volt", "temp", "power"};

u16_t ssi_handler(int iIndex, char *pcInsert, int iInsertLen) {
	size_t printed;
	switch (iIndex) {
	case 0: // volt
		{
			const float voltage = adc_read() * 3.3f / (1 << 12);
			printed = snprintf(pcInsert, iInsertLen, "%f", voltage);
		}
		break;
	case 1: // temp
		{
			const float voltage = adc_read() * 3.3f / (1 << 12);
			const float tempC = 27.0f - (voltage - 0.706f) / 0.001721f;
			printed = snprintf(pcInsert, iInsertLen, "%f", tempC);
		}
		break;
	case 2: // power
		{
			printed = snprintf(pcInsert, iInsertLen, "%d", current_state);
		}
		break;
	default:
		{
			printed = 0;
		}
		break;
	}

	return (u16_t) printed;
}

void ssi_init() {
	adc_init();
	adc_set_temp_sensor_enabled(true);
	adc_select_input(4);
	http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}

#endif