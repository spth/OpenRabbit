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

#include "myio.h"
#include "mytypes.h"
#include "bios/tc_defs.lib"
#include "bios/dkcore.lib"
#include "rabdata.h"
#include "rabio.h"

int rabbit_reset(int tty) {
	int s;

	// get current setting
	if(ioctl(tty, TIOCMGET, &s) < 0) {
		perror("ioctl(TIOCMGET)");
		return(-1);
	}

	fprintf(stderr, "reset rabbit\n");

	// set DTR (reset) high
	s |= TIOCM_DTR;
	if(ioctl(tty, TIOCMSET, &s) < 0) {
		perror("ioctl(TIOCMSET) high");
		return(-1);
	}
	
	// wait a bit
	usleep(250000);

	// set DTR (reset) low
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

	// open tty device, wihout controler
	if((tty = open(device, O_RDWR | O_NOCTTY)) < 0) {
		perror(device);
		return(tty);
	}

	return(tty);
}

char rabbit_write(int tty, uint8 type, uint8 subtype, uint16 length, void *data) {
	_TC_PacketHeader tcph;
	_TC_PacketFooter tcpf;
	uint8 framing;
	uint16 csum;
		
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
	csum = rabbit_csum(0, (uint8 *) &tcph, sizeof(tcph)-sizeof(tcph.header_checksum));
	tcph.header_checksum = csum;

	// send frame header
	if(rabbit_swrite(tty, &tcph, sizeof(tcph)) < (ssize_t)sizeof(tcph)) {
		perror("write(tcph) < sizeof(tcph)");
		return(-1);
	}

	// add checksum to checksum :)
	csum = rabbit_csum(csum, (uint8 *) &tcph.header_checksum, sizeof(tcph.header_checksum));

	// check for data frame
	if(data != NULL) {
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

char rabbit_poll(int tty, _TC_PacketHeader *tcph, uint16 length, void *data) {
	_TC_PacketFooter tcpf;
	uint8 framing;
	uint16 csum;
	char *b;

	// get frame start
	if(dread(tty, &framing, sizeof(framing)) < (ssize_t)sizeof(framing)) {
		perror("read(framing) < sizeof(framing)");
		return(1);
	}
	 
	// check framing
	if(framing != TC_FRAMING_START) {
		fprintf(stderr, "warning: framing failure, got 0x%02x\n", framing);
		return(1);
	}

	// get frame header
	if(rabbit_sread(tty, tcph, TC_HEADER_SIZE) < (ssize_t)TC_HEADER_SIZE) {
		perror("read(tcph) < TC_HEADER_SIZE");
		return(0);
	}

	// calculate frame checksum
	csum = rabbit_csum(0, (uint8 *) tcph, TC_HEADER_SIZE-sizeof(tcph->header_checksum));

	// check checksum
	if(csum != tcph->header_checksum) {
		fprintf(stderr, "error: header checksum mismatch 0x%02x != 0x%02x\n", csum, tcph->header_checksum);
		return(0);
	}

	// check for nak
	if(tcph->subtype & TC_NAK) {
		fprintf(stderr, "error: received NAK for subtype 0x%02x\n", tcph->subtype);
		return(0);
	}

	// remove NAK and ACK shit
	tcph->subtype &= ~(TC_NAK|TC_ACK);

	// add checksum to checksum :)
	csum = rabbit_csum(csum, (uint8 *) &tcph->header_checksum, sizeof(tcph->header_checksum));

	// check for data frame
	if(tcph->length > 0) {
		// get memory
		b = malloc(tcph->length);

		// read data
		if(rabbit_sread(tty, b, tcph->length) < tcph->length) {
			perror("read(b) < tcph->length");
			return(0);
		}

		// calculate data checksum
		csum = rabbit_csum(csum,  (uint8 *) b, tcph->length);

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
		return(0);
	}

	// check checksum
	if(csum != tcpf.checksum) {
		fprintf(stderr, "error: data checksum mismatch 0x%02x != 0x%02x\n", csum, tcpf.checksum);
		return(0);
	}

	return(1);
}

char rabbit_read(int tty, uint8 type, uint8 subtype, uint16 length, void *data) {
	_TC_PacketHeader tcph;

	// poll rabbit
	if(! rabbit_poll(tty, &tcph, length, data)) return(0);

	// check type
	if(type != tcph.type) {
		fprintf(stderr, "warning: type mismatch 0x%02x != 0x%02x\n", type, tcph.type);
	}

	// check subtype
	if(subtype != tcph.subtype) {
		fprintf(stderr, "warning: subtype mismatch 0x%02x != 0x%02x\n", subtype, tcph.subtype);
	}
	
	return(1);
}

int rabbit_coldload(int tty, const char *file) {
	const unsigned char coldload[6] = { 0x80, 0x50, 0x40, 0x80, 0x0E, 0x20 };
	const unsigned char colddone[6] = { 0x80, 0x0E, 0x30, 0x80, 0x24, 0x80 };
	unsigned char *pb = NULL;
	int sz;

	// load coldload.bin
	if((pb = load(pb, file, &sz)) == NULL) return(0);

	// set baudrate
	if(tty_setbaud(tty, 2400))
		return(-1);

	// tell rabbit coldload.bin is comming
	if(dwrite(tty, &coldload, sizeof(coldload)) < (ssize_t)sizeof(coldload)) {
		perror("write(coldload) < sizeof(coldload)");
		free(pb);
		return(-1);
	}

	// TODO: check status line, should be low now

	// send coldload.bin	FIXME: escape some chars???
	sz -= 3;
	fprintf(stderr, "sending %d coldload\n", sz);
	if(dwrite(tty, pb, sz) < sz) {
		perror("write(coldload) < sz");
		free(pb);
		return(-1);
	}

	// tell her we're done with coldload
	if(dwrite(tty, colddone, sizeof(colddone)) < (ssize_t)sizeof(colddone)) {
		perror("write(colddone) < sizeof(colddone)");
		free(pb);
		return(-1);
	}

	// TODO: check status line, should be high now

	return(0);
}

int rabbit_pilot(int tty, const char *pfile) {
	unsigned char *pb = NULL;
	uint16 csumR, csumU;
	uint8 csum;
	struct {
		uint32 off;
		uint16 sz;
		uint8 csum;
	} pilot;
	int sz, i;

	// move baudrate up
	if(tty_setbaud(tty, 57600))
		return(-1);

	// load pilot.bin
	if((pb = load(pb, pfile, &sz)) == NULL)
		return(-1);

	// tell her pilot.bin is comming
	pilot.off = 0x4000L;
	pilot.sz = sz - 0x6000L;	// pilot starts at 0x6000 in file? :S
	for(pilot.csum = 0, i = 0; i < 6; i++) pilot.csum += ((uint8*)&pilot)[i];
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
	fprintf(stderr, "sending %d pilot\n", pilot.sz);
	if(dwrite(tty, pb+0x6000L, pilot.sz) < pilot.sz) {
		perror("write(pilot) < pilot.sz");
		free(pb);
		return(-1);
	}

	// calculate pilot checksum
	csumU = rabbit_csum(0, pb+0x6000L, pilot.sz);

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

int rabbit_upload(int tty, const char *project) {
	unsigned char *pb = NULL;
	unsigned char *wp = NULL;
	_TCSystemInfoProbe info;
	unsigned char b[1024];
	uint32 baudrate;
	struct {
		uint16 sectorSize;
		uint16 numSectors;
		uint16 flashSize;
		uint16 writeMode;	// just a byte
	} flashdata;
	uint32 flash;
	int sz, i, l;
	int rs, ws;

	// request baudrate up
	baudrate = 115200;
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_SETBAUDRATE, sizeof(baudrate), &baudrate)) return(0);
	if(!rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_SETBAUDRATE, 0, NULL)) return(0);

	// move baudrate up
	if(tty_setbaud(tty, baudrate))
		return(-1);
	
	// probe for information
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_INFOPROBE, 0, NULL))
		return(-1);
	if(!rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_INFOPROBE, sizeof(b), &b))
		return(-1);
	rabbit_parse_info(&info, &b);

	// show some info
	fprintf(stderr, "CPU:  0x%04x\n", info.IDBlock.cpuID);
	fprintf(stderr, "Freq: %d\n", info.IDBlock.crystalFreq);
	fprintf(stderr, "sectorSize: 0x%04x\n", info.IDBlock.sectorSize);
	fprintf(stderr, "numSectors: 0x%04x\n", info.IDBlock.numSectors);
	fprintf(stderr, "flashSize:  0x%04x\n", info.IDBlock.flashSize);

	// send flashdata
	flashdata.sectorSize = info.IDBlock.sectorSize;
	flashdata.numSectors = info.IDBlock.numSectors;
	flashdata.flashSize = info.IDBlock.flashSize;
	flashdata.writeMode = 1;	// just a byte
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_FLASHDATA, sizeof(flashdata), &flashdata))
		return(-1);
	if(!rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_FLASHDATA, 0, NULL))
		return(-1);

	// load project.bin
	if((pb = load(pb, project, &sz)) == NULL)
		return(-1);

	// erase flash
	flash = WP_DATA_SIZE+sz;
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_ERASEFLASH, sizeof(flash), &flash))
		return(-1);
	if(!rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_ERASEFLASH, 0, NULL))
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
		memcpy(wp+1, wp+2, 6);

		// store data
		memcpy(wp+TC_SYSTEM_WRITE_HEADERSIZE, pb+i, l);

		// write packet
		if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_WRITE, TC_SYSTEM_WRITE_HEADERSIZE+l, wp))
			return(-1);
		if(!rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_WRITE, 0, NULL))
			return(-1);
	}

	fprintf(stderr, "sending %s... done\n", project);

	return(0);
}

char rabbit_debug(int tty) {
	uint16 debugtag;
	uint8 sendflags;
	uint8 start;

	fprintf(stderr, "starting the bios\n");

	// start the bios
	start = TC_STARTBIOS_FLASH;
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_STARTBIOS, sizeof(start), &start)) return(0);
	/* no reply for this one? :S */

	usleep(100000);

	// ping her once
	fprintf(stderr, "ping?\n");
	if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_NOOP, 0, NULL)) return(0);
	if(!rabbit_read(tty, TC_TYPE_SYSTEM, TC_SYSTEM_NOOP, 0, NULL)) return(0);
	fprintf(stderr, "pong!\n");

	fprintf(stderr, "configure debug mode\n");

	// set debug tag
	debugtag = 0x82;
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SETDEBUGTAG, sizeof(debugtag), &debugtag)) return(0);
	if(!rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_SETDEBUGTAG, 0, NULL)) return(0);

	// set send flags
	sendflags = 0x08;
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SETSENDFLAGS, sizeof(sendflags), &sendflags)) return(0);
	if(!rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_SETSENDFLAGS, 0, NULL)) return(0);

	// start from begin
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_STARTPROGRAM, 0, NULL)) return(0);
	if(!rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_REGDATA, 0, NULL)) return(0);
	if(!rabbit_read(tty, TC_TYPE_DEBUG, TC_DEBUG_STARTPROGRAM, 0, NULL)) return(0);

	return(1);
}

char rabbit_program(int tty, char *coldload, char *pilot, char *project) {
	// reset her
	if(rabbit_reset(tty))
		return(-1);

	// coldload her
	if(rabbit_coldload(tty, coldload))
		return(-1);

	// load pilot
	if(rabbit_pilot(tty, pilot))
		return(-1);

	// load project
	if(rabbit_upload(tty, project))
		return(-1);

	return(0);
}

