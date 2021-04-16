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
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "myio.h"

// #define DEBUG_COMM
// #define DEBUG_IO

extern int verbose;

unsigned long dwrite_count = 0;
unsigned long dread_count = 0;
unsigned long long drw_time = 0;

bool fileext_is(const char *filename, const char *ext) {
	char *ext_begin = strrchr(filename, '.');
	return(ext_begin && strcmp(ext_begin, ext) == 0);
}

ssize_t dwrite(int fd, const void *buf, size_t count) {
	ssize_t r;
#ifdef DEBUG_COMM
	fprintf(stderr, "writing %d bytes\n", count);
#endif
#ifdef DEBUG_IO
	write(1, buf, count);
#endif
	r = write(fd, buf, count);
	if(r > 0) dwrite_count += r;
	return(r);
}

ssize_t dread(int fd, void *buf, size_t count) {
	ssize_t r, c;
#ifdef DEBUG_COMM
	fprintf(stderr, "reading %d bytes\n", count);
#endif
	for(c = 0; c < count; c += r) {
		r = read(fd, (char *)buf+c, count-c);
 		if(r < 0) return(r);
	}
#ifdef DEBUG_COMM
	fprintf(stderr, "read %d bytes\n", c);
#endif
#ifdef DEBUG_IO
	write(1, buf, c);
#endif
	dread_count += c;
	return(c);
}

unsigned long long dtime(void) {
	unsigned long long d;
	struct timeval t;
	gettimeofday(&t,NULL);
	d = (t.tv_sec * 1000) + (t.tv_usec / 1000);
	return(d);
}

// FIXME: counters should be fd dependent...?
void dtiming(int *rs, int *ws) {
	unsigned long long d;
	long e;

	d = dtime();
	e = d - drw_time;

	if(e < 0) {
		drw_time = d;
	} else if(e > 200) {
		*ws = ((dwrite_count*1000) / e);
		*rs = ((dread_count*1000) / e);

		if(e > 2000) {
			dwrite_count = 0;
			dread_count = 0;
			drw_time = d;
		}
	}
}

unsigned char *load(unsigned char *pb, const char *file, int *sz) {
	struct stat s;
	int fd, r;

	// open file
	if((fd = open(file, O_RDONLY)) < 0) {
		perror(file);
		if(pb)
			free(pb);
		return(NULL);
	}

	// get size
	if(fstat(fd, &s) < 0) {
		perror(file);
		if(pb)
			free(pb);
		goto load_ret;
	}

	// allocate memory
	if(!(pb = realloc(pb, s.st_size))) {
		perror(file);
		goto load_ret;
	}

	// read file
	r = read(fd, pb, s.st_size);
	
	// check amount
	if(r < s.st_size) {
		perror(file);
		if(pb)
			free(pb);
		goto load_ret;
	}

	// store amount
	*sz = r;

	if(verbose)
		fprintf(stderr, "loaded %d bytes from %s\n", r, file);

load_ret:
	close(fd);
	return(pb);
}

int tty_setbaud(int tty, unsigned long baud) {
	struct termios newtio;
	int b;

	// convert baudrate
	switch(baud) {
	case 2400:
		b = B2400;
		break;
	case 19200:
		b = B19200;
		break;
	case 38400:
		b = B38400;
		break;
	case 57600:
		b = B57600;
		break;
	case 115200:
		b = B115200;
		break;
	case 230400:
		b = B230400;
		break;
	case 460800:
		b = B460800;
		break;
	default:
		fprintf(stderr, "invalid baud in setbaud(), left unmodified!\n");
		return(-1);
	}
  
	// setup port
	newtio.c_cflag = b | CS8 | CLOCAL | CREAD | CSTOPB;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_lflag = 0;
	newtio.c_cc[VMIN] = 1;
	newtio.c_cc[VTIME] = 0;

	if(verbose > 2)
		fprintf(stderr, "flushing data for baudrate set\n");

	// change settings
	if(tcsetattr(tty, TCSAFLUSH, &newtio) < 0)
		return(-1);

	if(verbose > 1)
		fprintf(stderr, "set baudrate to %lu\n", baud);

	// give other end time
	usleep(100000);

	return(0);
}

