#include <stdio.h>
#include <stdint.h>

#include "r2k.h"

unsigned long clock(void);

void init(void)
{
	PCFR = 0x40;	// Use pin PC6 as TXA
	TAT4R = 18 - 1;	// use divider for 38400 baud - value in register is one less than the divider used (e.g. a value of 0 will result in clock division by 1).
	TACSR = 0x01;	// Enable timer A
	SACR = 0x00;	// No interrupts, 8-bit async mode
}

unsigned long clock(void)
{
	unsigned long clock0, clock1;
	do
	{
		RTC0R = 0;
		clock0 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
		clock1 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
	} while (clock0 != clock1);
	return(clock1);
}

int putchar(int c)
{
	// Convert newline to CRLF
	if (c == '\n') {
		putchar('\r');
	}

	while (SASR & 0x04);	// Wait for empty transmitter data register
	SADR = c;
	return c;
}

