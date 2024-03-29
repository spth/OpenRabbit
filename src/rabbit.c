/***************************************************************************
 *   Copyright (C) 2004 by Lourens Rozema                                  *
 *   ik@lourensrozema.nl                                                   *
 *   Copyright (C) 2020 by Philipp Klaus Krause                            * 
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#define WP_DATA_ORG 0x80000L
// #define WP_DATA_SIZE 0xF0			// this makes it faster, but may damage things?
#define WP_DATA_SIZE 0x80

#define RFU_VERSION 0x02

#include "rabbit.h"
#include "myio.h"
#include "mytypes.h"
#include "bios/tc_defs.lib"
#include "bios/dkcore.lib"
#include "rabdata.h"
#include "rabio.h"
#include "ihex.h"

int rabbit_reset(int tty) {
	int s;

	// get current setting
	if(ioctl(tty, TIOCMGET, &s) < 0) {
		perror("ioctl(TIOCMGET)");
		return(-1);
	}

	if(verbose)
		fprintf(stderr, "Reset Rabbit.\n");

	// Assert DTR (i.e drive /reset low)
	s |= TIOCM_DTR;
	if(ioctl(tty, TIOCMSET, &s) < 0) {
		perror("ioctl(TIOCMSET) high");
		return(-1);
	}

	// wait a bit
	usleep(400000); // Originally, this was a 250 ms wait. But reset was often unreliable. The 400 ms wait works much better.

	// Deassert DTR (i.e drive /reset high)
	s &= ~TIOCM_DTR;
	if(ioctl(tty, TIOCMSET, &s) < 0) {
		perror("ioctl(TIOCMSET) low");
		return(-1);
	}
  
	// give her time
	usleep(250000);

	return(0);
}

int rabbit_open(const char *device) {
	int tty;

	// open tty device, without controler
	if((tty = open(device, O_RDWR | O_NOCTTY)) < 0) {
		perror(device);
		return(tty);
	}

	return(tty);
}

char rabbit_write(int tty, uint8_t type, uint8_t subtype, uint16_t length, void *data) {
	_TC_PacketHeader tcph;
	_TC_PacketFooter tcpf;
	uint8_t framing;
	uint16_t csum;
		
	// send frame start
	framing = TC_FRAMING_START;
	if(dwrite(tty, &framing, sizeof(framing)) < (ssize_t)sizeof(framing)) {
		perror("write(framing) < sizeof(framing)");
		return(-1);
	}

	// create frame header
	tcph.version = RFU_VERSION;
	tcph.flags = 0;
	tcph.type = type;
	tcph.subtype = subtype;
	tcph.length = length;

	// calculate frame checksum
	csum = rabbit_csum(0, (uint8_t *) &tcph, sizeof(tcph)-sizeof(tcph.header_checksum));
	tcph.header_checksum = csum;

	// send frame header
	if(rabbit_swrite(tty, &tcph, sizeof(tcph)) < (ssize_t)sizeof(tcph)) {
		perror("write(tcph) < sizeof(tcph)");
		return(-1);
	}

	// add checksum to checksum :)
	csum = rabbit_csum(csum, (uint8_t *) &tcph.header_checksum, sizeof(tcph.header_checksum));

	// check for data frame
	if(data) {
		// write data
		if(rabbit_swrite(tty, data, length) < length) {
			perror("write(data) < length");
			return(0);
		}
		
		// calculate data checksum
		csum = rabbit_csum(csum, data, length);
	}

	// create frame footer
	tcpf.checksum = csum;

	// send frame footer
	if(rabbit_swrite(tty, &tcpf, sizeof(tcpf)) < (ssize_t)sizeof(tcpf)) {
		perror("write(tcpf) < sizeof(tcpf)");
		return(0);
	}

	return(1);
}

// Returns 0 on success, -1 on error, 1 on nak.
int rabbit_poll(int tty, _TC_PacketHeader *tcph, uint16_t length, void *data) {
	_TC_PacketFooter tcpf;
	uint8_t framing;
	uint16_t csum;
	char *b;
	bool nak = false;

	// get frame start
	if(dread(tty, &framing, sizeof(framing)) < (ssize_t)sizeof(framing)) {
		perror("read(framing) < sizeof(framing)");
		return(0);
	}
	 
	// check framing
	if(framing != TC_FRAMING_START) {
		fprintf(stderr, "warning: framing failure, got 0x%02x\n", framing);
		return(0);
	}

	// get frame header
	if(rabbit_sread(tty, tcph, TC_HEADER_SIZE) < (ssize_t)TC_HEADER_SIZE) {
		perror("read(tcph) < TC_HEADER_SIZE");
		return(-1);
	}

	// calculate frame checksum
	csum = rabbit_csum(0, (uint8_t *) tcph, TC_HEADER_SIZE-sizeof(tcph->header_checksum));

	// check checksum
	if(csum != tcph->header_checksum) {
		fprintf(stderr, "error: header checksum mismatch 0x%02x != 0x%02x\n", csum, tcph->header_checksum);
		return(-1);
	}

	// check for nak
	if(tcph->subtype & TC_NAK) {
		if(verbose > 1)
			fprintf(stderr, "warning: received NAK for subtype 0x%02x\n", tcph->subtype);
		nak = true;
	}

	// remove NAK and ACK shit
	tcph->subtype &= ~(TC_NAK|TC_ACK);

	// add checksum to checksum :)
	csum = rabbit_csum(csum, (uint8_t *) &tcph->header_checksum, sizeof(tcph->header_checksum));

	// check for data frame
	if(tcph->length > 0) {
		// get memory
		b = malloc(tcph->length);

		// read data
		if(rabbit_sread(tty, b, tcph->length) < tcph->length) {
			perror("read(b) < tcph->length");
			return(-1);
		}

		// calculate data checksum
		csum = rabbit_csum(csum,  (uint8_t *) b, tcph->length);

		if(data != NULL) {
			// report space problem
			if(length < tcph->length) {
				fprintf(stderr, "warning: data length mismatch %d < %d\n", length, tcph->length);
			}

			// copy data
			memcpy(data, b, tcph->length<length?tcph->length:length);

			// release buffer
			free(b);
		} else {
			// fprintf(stderr, "warning: unwanted data for subtype 0x%02x\n", tcph->subtype);
		}
	} else if(data != NULL) {
		// fprintf(stderr, "warning: no data returned for subtype 0x%02x\n", tcph->subtype);
	}

	// create frame footer
	tcpf.checksum = csum;

	// get frame footer
	if(rabbit_sread(tty, &tcpf, sizeof(tcpf)) < (ssize_t)sizeof(tcpf)) {
		perror("read(tcpf) < sizeof(tcpf)");
		return(-1);
	}

	// check checksum
	if(csum != tcpf.checksum) {
		fprintf(stderr, "error: data checksum mismatch 0x%02x != 0x%02x\n", csum, tcpf.checksum);
		return(-1);
	}

	return(nak);
}

// Returns 0 on success, -1 on error, 1 on nak.
int rabbit_read(int tty, uint8_t type, uint8_t subtype, uint16_t length, void *data) {
	_TC_PacketHeader tcph;
	int ret;

	// poll rabbit
	if((ret = rabbit_poll(tty, &tcph, length, data)) < 0)
		return(-1);

	// check type
	if(type != tcph.type) {
		fprintf(stderr, "warning: type mismatch 0x%02x != 0x%02x\n", type, tcph.type);
	}

	// check subtype
	if(subtype != tcph.subtype) {
		fprintf(stderr, "warning: subtype mismatch 0x%02x != 0x%02x\n", subtype, tcph.subtype);
	}
	
	return(ret);
}

static int rabbit_triplet(int tty, const unsigned char triplet[3]) {
	if(dwrite(tty, triplet, 3) < 3) {
		perror("triplet write < 3");
		return(-1);
	}
	return(0);
}

int rabbit_triplets(int tty, const unsigned char *triplets, int n) {
	for(int i = 0; i < n; i++)
		if(rabbit_triplet(tty, triplets + i * 3))
			return(-1);

	// Ensure the triplets have actually been sent. We used to always do this with an usleep(15000) per triplet instead, but using tcdrain() is faster. However, apparently tcdrain() doesn't work reliably with many USB-to-serial coverters.
	if(tcdrain(tty)) {
		perror("failed to drain serial buffers");
		return(-1);
	}
	for (unsigned int s = 0; s < slow; s++)
		usleep(15000ul * n);

	return(0);
}

int rabbit_coldload(int tty, const char *file) {
	int s;
	const unsigned char pverify[12] = { 0x80, 0x09, 0x51, 0x80, 0x09, 0x54, 0x80, 0x0e, 0x30, 0x80, 0x0e, 0x20};
	const unsigned char coldload[6] = { 0x80, 0x50, 0x40, 0x80, 0x0e, 0x20 };
	const unsigned char colddone[6] = { 0x80, 0x0e, 0x30, 0x80, 0x24, 0x80 };
	unsigned char *pb = 0;
	int sz;
	bool pverify_failed = false;

	// Load initial loader (binary consisting of triplets).
	if(!(pb = load(pb, file, &sz)))
		return(-1);

	if(sz % 3) {
		fprintf(stderr, "Initial loader triplet binary %s size is not a multiple of 3.\n", file);
		return(-1);
	}

	fprintf(stderr, "Sending initial loader.\n");

	// Set baudrate.
	if(tty_setbaud(tty, 2400))
		return(-1);

	// Processor verifiction sequence.
	if(rabbit_triplets(tty, pverify, 3))
		return(-1);
	usleep(100000);
	if(ioctl(tty, TIOCMGET, &s) < 0) {
		perror("ioctl(TIOCMGET)");
		free(pb);
		return(-1);
	}
	pverify_failed += (s & TIOCM_DSR);
	if(rabbit_triplets(tty, pverify + 9, 1))
		return(-1);
	usleep(100000);
	if(ioctl(tty, TIOCMGET, &s) < 0) {
		perror("ioctl(TIOCMGET)");
		free(pb);
		return(-1);
	}
	pverify_failed += !(s & TIOCM_DSR);
	if(pverify_failed)
		fprintf(stderr, "Warning: Processor verification sequence failed!\n");

	// tell rabbit initial loader is comming.
	if(rabbit_triplets(tty, coldload, sizeof(coldload) / 3)) {
		free(pb);
		return(-1);
	}

	usleep (100000);
	// Check status line.
	if(ioctl(tty, TIOCMGET, &s) < 0) {
		perror("ioctl(TIOCMGET)");
		free(pb);
		return(-1);
	}
	if(!(s & TIOCM_DSR)) {
		fprintf(stderr, "Error: Status line should be low before sending initial loader.\n");
		free(pb);
		return(-1);
	}

	// Send initial loader.
	sz -= 3; // Skip 0x80, 0x24, 0x80 at end of initial loader.
	if (verbose)
		fprintf(stderr, "sending %d initial loader triplets\n", sz / 3);
	if(rabbit_triplets(tty, pb, sz / 3)) {
		free(pb);
		return(-1);
	}
	free(pb);

	// Tell her we're done with initial loader.
	if(rabbit_triplets(tty, colddone, sizeof(colddone) / 3)) {
		free(pb);
		return(-1);
	}

	usleep (100000);
	// Check status line.
	if(ioctl(tty, TIOCMGET, &s) < 0) {
		perror("ioctl(TIOCMGET)");
		return(-1);
	}
	if(s & TIOCM_DSR ) {
		fprintf(stderr, "Error: Status line should be high after sending initial loader.\n");
		return(-1);
	}

	return(0);
}

int rabbit_pilot(int tty, const char *pfile, bool *dc8pilot) {
	unsigned char *pb = NULL;
	uint16_t csumR, csumU;
	uint8_t csum;
	struct {
		uint32_t off;
		uint16_t sz;
		uint8_t csum;
	} pilot;
	int sz, i;

	// move baudrate up
	if(tty_setbaud(tty, 57600))
		return(-1);

	// load pilot.bin
	if((pb = load(pb, pfile, &sz)) == NULL)
		return(-1);

	*dc8pilot = sz > 0x6000;
	for(int i = 0; i < 0x6000; i++)
		if (pb[i])
			*dc8pilot = false;

	int pilotoffset = *dc8pilot ? 0x6000 : 0;

	fprintf(stderr, "Secondary loader format detected as %s\n", *dc8pilot ? "Dynamic C 8" : "Dynamic C 9");

	fprintf(stderr, "Sending secondary loader.\n");

	// tell her pilot.bin is comming
	pilot.off = 0x4000L;
	pilot.sz = sz - pilotoffset;
	for(pilot.csum = 0, i = 0; i < 6; i++) pilot.csum += ((uint8_t*)&pilot)[i];
	if(dwrite(tty, &pilot, 7) < 7) {
		perror("write(pilot) < sizeof(pilot)");
		free(pb);
		return(-1);
	}

	// wait for checksum
	if(dread(tty, &csum, sizeof(csum)) < (ssize_t)sizeof(csum)) {
		perror("read(csum) < sizeof(csum)");
		free(pb);
		return(-1);
	}

	// check csum
	if(pilot.csum != csum) {
		fprintf(stderr, "pilot.csum 0x%02x != csum 0x%02x\n", pilot.csum, csum);
		free(pb);
		return(-1);
	}

	// send pilot
	if(verbose)
		fprintf(stderr, "sending %d secondary loader bytes.\n", pilot.sz);
	if(dwrite(tty, pb + pilotoffset, pilot.sz) < pilot.sz) {
		perror("write(pilot) < pilot.sz");
		free(pb);
		return(-1);
	}

	// calculate pilot checksum
	csumU = rabbit_csum(0, pb + pilotoffset, pilot.sz);

	// wait for checksum
	if(dread(tty, &csumR, sizeof(csumR)) < (ssize_t)sizeof(csumR)) {
		perror("read(csumR) < sizeof(csumR)");
		free(pb);
		return(-1);
	}
		
	// check csum1,2
	if(csumR != csumU) {
		fprintf(stderr, "csumR 0x%04x != csumU 0x%04x\n", csumR, csumU);
		free(pb);
		return(-1);
	}

	// give her time to boot pilot
	usleep(100000);
	return(0);
}

int rabbit_upload(int tty, const char *project, bool dc8pilot) {
	unsigned char *pb = NULL;
	unsigned char *wp = NULL;
	_TCSystemInfoProbe info;
	unsigned char b[1024];
	unsigned long baudrate;
	struct {
		uint16_t sectorSize;
		uint16_t numSectors;
		uint16_t flashSize;
		uint16_t writeMode;	// just a byte
	} flashdata;
	uint32_t flash;
	int sz, i, l;
	int rs, ws;

	if(verbose)
		fprintf(stderr, "Negotiating baudrate.\n");

	// Request baudrate up (host side).
	baudrate = dc8pilot ? 115200 : 460800;

	while(tty_setbaud(tty, baudrate)) { // Find maximum baudrate on our side.
		baudrate /= 2;
		if(baudrate < 57600) {
			fprintf(stderr, "Failed to set baudrate for host port.\n");
			return(-1);
		}
	}
	tty_setbaud(tty, 57600); // Go back to lower speed to tell Rabbit to increase speed.

	// Request baudrate up (Rabbit side).
	for(;;) { // Find maximum baudrate on Rabbit side.
		if(baudrate < 57600) {
			fprintf(stderr, "Failed to set baudrate for Rabbit port.\n");
			return(-1);
		}

		if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_SETBAUDRATE, sizeof(baudrate), &baudrate))
			return(-1);
		int r = rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_SETBAUDRATE, 0, NULL);
		if(r < 0)
			return(-1);
		else if(!r)
			break;
		baudrate /= 2;
	}

	// Finally switch to higher speed.
	if(tty_setbaud(tty, baudrate))
		return(-1);

	// probe for information
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_INFOPROBE, 0, NULL))
		return(-1);
	if(rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_INFOPROBE, sizeof(b), &b))
		return(-1);
	rabbit_parse_info(&info, dc8pilot ? b : b + 4); // DC9 prepends a 4 byte system id block.

	// show some info
	fprintf(stderr, "CPU:  0x%04x (%s)\n", info.IDBlock.cpuID, rabbit_cpuname(info.IDBlock.cpuID));
	fprintf(stderr, "Freq: %d\n", info.IDBlock.crystalFreq);
	fprintf(stderr, "sectorSize: 0x%04x\n", info.IDBlock.sectorSize);
	fprintf(stderr, "numSectors: 0x%04x\n", info.IDBlock.numSectors);
	fprintf(stderr, "flashSize:  0x%04x\n", info.IDBlock.flashSize);

	if(verbose) {
		fprintf(stderr, "Flash speed: %d ns\n", info.IDBlock.flashSpeed);
		fprintf(stderr, "RAM speed: %d ns\n", info.IDBlock.ramSpeed);
	}

	// send flashdata
	flashdata.sectorSize = info.IDBlock.sectorSize;
	flashdata.numSectors = info.IDBlock.numSectors;
	flashdata.flashSize = info.IDBlock.flashSize;
	flashdata.writeMode = 1;	// just a byte
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_FLASHDATA, sizeof(flashdata), &flashdata))
		return(-1);
	if(rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_FLASHDATA, 0, NULL))
		return(-1);

	// load project.bin
	bool ihex_format = fileext_is (project, ".ihx") || fileext_is (project, ".hex");
	if (ihex_format) {
		if (verbose)
			fprintf (stderr, "Due to its file extension, \"%s\" is considered to be in Intel hex format (of up to 256 KB size).\n", project);
		FILE *f = fopen(project, "r");
		if (!f) {
			fprintf (stderr, "Failed to open file %s.\n", project);
			return(-1);
		}
		pb = malloc(256 * 1024);
		sz = ihex_read(f, pb, 0, 256 * 1024);
		fclose(f);
		if (sz < 0)
			return(-1);
	}
	else {
		if((pb = load(pb, project, &sz)) == NULL)
			return(-1);
	}

	// erase flash
	flash = WP_DATA_SIZE+sz;
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_ERASEFLASH, sizeof(flash), &flash))
		return(-1);
	if(rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_ERASEFLASH, 0, NULL))
		return(-1);

	// allocate memory
	wp = malloc(TC_SYSTEM_WRITE_HEADERSIZE+WP_DATA_SIZE);

	// write project.bin
	for(i = 0; i < sz; i += l) {
		dtiming(&rs, &ws);
		fprintf(stderr, "sending %s... %d%% (bps: in=%d, out=%d)                   \r", project, !i?0:(i*100/sz), rs, ws);
		fflush(stderr);

		// calculate length left
		l = (sz-i)<WP_DATA_SIZE?(sz-i):WP_DATA_SIZE;

		// create write header
		((_TCSystemWRITE*)wp)->type = TC_SYSWRITE_PHYSICAL;
		((_TCSystemWRITE*)wp)->length = l;
		((_TCSystemWRITE*)wp)->address.physical = WP_DATA_ORG + i;
		
		// fix alignment
		memmove(wp+1, wp+2, 6);

		// store data
		memcpy(wp+TC_SYSTEM_WRITE_HEADERSIZE, pb+i, l);

		// write packet
		if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_WRITE, TC_SYSTEM_WRITE_HEADERSIZE+l, wp))
			return(-1);
		if(rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_WRITE, 0, NULL))
			return(-1);
	}

	fprintf(stderr, "sending %s... done\n", project);

	return(0);
}

char rabbit_debug(int tty) {
	uint16_t debugtag;
	uint8_t sendflags;
	uint8_t start;

	fprintf(stderr, "starting the bios\n");

	// start the bios
	start = TC_STARTBIOS_FLASH;
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_STARTBIOS, sizeof(start), &start)) return(0);
	/* no reply for this one? :S */

	usleep(100000);

	// ping her once
	fprintf(stderr, "ping?\n");
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_NOOP, 0, NULL)) return(0);
	if(rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_NOOP, 0, NULL))
		return(0);
	fprintf(stderr, "pong!\n");

	fprintf(stderr, "configure debug mode\n");

	// set debug tag
	debugtag = 0x82;
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SETDEBUGTAG, sizeof(debugtag), &debugtag)) return(0);
	if(rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_SETDEBUGTAG, 0, NULL))
		return(0);

	// set send flags
	sendflags = 0x08;
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SETSENDFLAGS, sizeof(sendflags), &sendflags)) return(0);
	if(rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_SETSENDFLAGS, 0, NULL))
		return(0);

	// start from begin
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_STARTPROGRAM, 0, NULL)) return(0);
	if(rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_REGDATA, 0, NULL))
		return(0);
	if(rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_STARTPROGRAM, 0, NULL))
		return(0);

	return(1);
}
#include <time.h>
int rabbit_program(int tty, const char *coldload, const char *pilot, const char *project, bool *dc8pilot) {
	// reset her
	if(rabbit_reset(tty))
		return(-1);

	// coldload her
	if(rabbit_coldload(tty, coldload))
		return(-1);

	// load pilot
	if(rabbit_pilot(tty, pilot, dc8pilot))
		return(-1);

	// load project
	if(rabbit_upload(tty, project, *dc8pilot))
		return(-1);

	return(0);
}

int rabbit_start(int tty)
{
	const unsigned char start[3] = { 0x80, 0x24, 0x80};

	// Set baudrate back to 2400
	if(tty_setbaud(tty, 2400))
		return(-1);

	if(rabbit_reset(tty))
		return(-1);

	usleep(250000); // Hack: the number 250000 here is just a guess. Without the usleep, we often hang, with usleep (50000) sometimes.

	if(rabbit_triplets(tty, start, sizeof(start) / 3))
		return(-1);
}

