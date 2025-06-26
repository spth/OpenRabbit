/*
   Copyright (c) 2020 Digi International Inc.
   Copyright (c) 2025 Philipp Klaus Krause

   This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

/*
 *		makecold.c - written in the common subset of ISO C99 and ISO C23.
 *		This program builds a "tripletized" coldloader file from
 *		a *.bin file. (jjb)
 *
 *		Select the desired combination of RAM control lines and spectrum
 *		spreader options, below.
 */

// RAM control lines:  0 is /OE0/WE0/CS0, 1 is /OE1/WE1/CS1, 2 is /OE1/WE1/CS2,
//                     3 is /OE1/WE1/CS0, 4 is /OE0/WE0/CS1, 5 is /OE0/WE0/CS2
//                    -1 lets openrabbitfu provide the RAM configuration
#define RAM_CONTROL_OPTION -1

// spectrum spreader:  0 to disable, 1 for normal, 2 for strong
#define ENABLE_SPREADER_OPTION 0

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if 0 == RAM_CONTROL_OPTION
	#define RC0_SEQUENCE "\x80\x14\x40"	// MB0CR: 2 ws, /OE0, /WE0, /CS0 active
	#define RC1_SEQUENCE "\x80\x15\x40"	// MB1CR: 2 ws, /OE0, /WE0, /CS0 active
#elif 1 == RAM_CONTROL_OPTION
	#define RC0_SEQUENCE "\x80\x14\x45"	// MB0CR: 2 ws, /OE1, /WE1, /CS1 active
	#define RC1_SEQUENCE "\x80\x15\x45"	// MB1CR: 2 ws, /OE1, /WE1, /CS1 active
#elif 2 == RAM_CONTROL_OPTION
	#define RC0_SEQUENCE "\x80\x14\x46"	// MB0CR: 2 ws, /OE1, /WE1, /CS2 active
	#define RC1_SEQUENCE "\x80\x15\x46"	// MB1CR: 2 ws, /OE1, /WE1, /CS2 active
#elif 3 == RAM_CONTROL_OPTION
	#define RC0_SEQUENCE "\x80\x14\x44"	// MB0CR: 2 ws, /OE1, /WE1, /CS0 active
	#define RC1_SEQUENCE "\x80\x15\x44"	// MB1CR: 2 ws, /OE1, /WE1, /CS0 active
#elif 4 == RAM_CONTROL_OPTION
	#define RC0_SEQUENCE "\x80\x14\x41"	// MB0CR: 2 ws, /OE0, /WE0, /CS1 active
	#define RC1_SEQUENCE "\x80\x15\x41"	// MB1CR: 2 ws, /OE0, /WE0, /CS1 active
#elif 5 == RAM_CONTROL_OPTION
	#define RC0_SEQUENCE "\x80\x14\x42"	// MB0CR: 2 ws, /OE0, /WE0, /CS2 active
	#define RC1_SEQUENCE "\x80\x15\x42"	// MB1CR: 2 ws, /OE0, /WE0, /CS2 active
#endif

#if 0 == ENABLE_SPREADER_OPTION
	// OK, but spectrum spreader is disabled.
#elif 1 == ENABLE_SPREADER_OPTION
	#define ES_SEQUENCE "\x80\x0A\x00"	// GCM0R: normal spreading
#elif 2 == ENABLE_SPREADER_OPTION
	#define ES_SEQUENCE "\x80\x0A\x80"	// GCM0R: strong spreading
#else
	#error "Unsupported spectrum spreader option!"
#endif

/*
 *	This is the first few bytes of the coldload sequence.  It is
 *	used to set up some of the configuration registers.
 */

const char start_sequence[] =
#if 0 < ENABLE_SPREADER_OPTION
	"\x80\x0E\xA0"  // GOCR: CLK and STATUS low, /WDTOUB and /BUFEN active low
	"\x80\x19\x0C"  // MTCR: /OE0, /OE1 to toggle 1/2 clock early
	ES_SEQUENCE
	"\x80\x0B\x80"  // GCM1R: spreader on
#endif
	"\x80\x00\x08"  // GCSR: no periodic int, main osc no div
	"\x80\x10\x00"  // MMIDR: normal operation
#if 0 <= RAM_CONTROL_OPTION
	RC0_SEQUENCE
	RC1_SEQUENCE
#endif
	"\x80\x16\x40"  // MB2CR: 2 ws, /OE0, /WE0, /CS0 active
	"\x80\x17\x40"  // MB3CR: 2 ws, /OE0, /WE0, /CS0 active
	"\x80\x13\xc6"  // SEGSIZE
	"\x80\x11\x74"  // STACKSEG
	"\x80\x12\x3a"; // DATASEG

int main(int argc, char* argv[])
{
	FILE *h_in, *h_out;
	unsigned int x;
	char seq[3];

	if(argc != 3)
	{
		const char *ptr2me;
		// the following '\\' assumes Windows style pathlist separator!
		if((ptr2me = strrchr(argv[0], '\\')) == NULL)
			ptr2me = argv[0]; // just use the whole string
		else
			ptr2me++; // skip the pathlist separator
		fprintf(stderr, "usage: %s <source file> <destination file>\n", ptr2me);
		return 1;
	}

	printf("source=%s, destination=%s\n", argv[1], argv[2]);

	if(!(h_in = fopen(argv[1], "rb")))
	{
		perror("error opening source file\n");
		return 1;
	}

	if(!(h_out = fopen(argv[2], "wb")))
	{
		perror("error creating destination file\n");
		return 1;
	}

	if(fwrite(start_sequence, sizeof(start_sequence) - 1, 1, h_out) != 1)
	{
		fprintf(stderr, "error writing start sequence\n");
	}

	for(x = 0; x < 32768; x++)
	{
		char ch;
		int r = fread(&ch, 1, 1, h_in);
		if(ferror(h_in))
		{
			fprintf(stderr, "error reading input file\n");
			return 1;
		}
		if(!r)
			break;

		seq[0] = (x >> 8) & 0xff;
		seq[1] = x & 0xff;
		seq[2] = ch;

		if(fwrite(seq, 3, 1, h_out) != 1)
		{
			fprintf(stderr, "error writing output file\n");
			return 1;
		}
	}

	seq[0] = 0x80;
	seq[1] = 0x24;
	seq[2] = 0x80;

	if(fwrite(seq, 3, 1, h_out) != 1)
	{
		fprintf(stderr, "error writing 0x80 0x24 0x80\n");
		return 1;
	}

	if(x >= 32768)
	{
		fprintf(stderr, "error input file too long\n");
	}

	printf("output file successfully created (%u bytes)\n", (int)(x * 3 + sizeof(start_sequence) - 1));
	return 0;
}

