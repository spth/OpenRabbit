#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "r2k.h"

#include "targetconfigurations.h"

void _sdcc_external_startup(void)
{
	// Disable watchdog
	WDTTR = 0x51;
	WDTTR = 0x54;

	// normal oscillator, processor and peripheral from main clock, no periodic interrupt
	GCSR = 0x08;

	GCDR = CLOCK_DOUBLER; // If possible, double clock to get more speed

	// Configure memory wait states
	MB0CR = MB0CR_VALUE;
	MB2CR = MB2CR_VALUE;
}

void init(void)
{
	PCFR = 0x40;                      // Use pin PC6 as TXA
	TAT4R = SERIAL_DIVIDER_38400 - 1; // Use divider for 38400 baud - value in register is one less than the divider used (e.g. a value of 0 will result in clock division by 1).
	TACSR = 0x01;                     // Enable timer A
	SACR = 0x00;                      // No interrupts, 8-bit async mode
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

