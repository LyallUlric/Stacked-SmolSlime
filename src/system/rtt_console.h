#include <SEGGER_RTT.h>
#include <stdio.h>

static uint8_t line[64];

static uint8_t* rtt_console_getline(void)
{
	uint8_t len = 0;
	do {
		while (SEGGER_RTT_HasKey() == 0)
			k_usleep(1);
		line[len] = SEGGER_RTT_GetKey();
		printk("%c", line[len]);
		len++;
	} while (len < sizeof(line) && line[len - 1] != '\n' && line[len - 1] != '\r');
	line[len - 1] = 0;
	return line;
}
