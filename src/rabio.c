/***************************************************************************
 *   Copyright (C) 2004 by Lourens Rozema                                  *
 *   ik@lourensrozema.nl                                                   *
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "myio.h"
#include "mytypes.h"
#include "bios/tc_defs.lib"

ssize_t rabbit_swrite(int tty, void *data, size_t length) {
	char *d = data;
	char *b = NULL;
	int c, i;

	// count 7Ds and 7Es
	for(c = 0, i = 0; i < length; i++)
		if(d[i] == TC_FRAMING_START || d[i] == TC_FRAMING_ESC) c++;

	// get a buffer
	b = malloc(length + c);

	// rewrite data
	for(c = 0, i = 0; i < length; i++) {
		if(d[i] == TC_FRAMING_START || d[i] == TC_FRAMING_ESC) {
			b[c++] = TC_FRAMING_ESC;
			b[c++] = d[i] ^ 0x20;
		} else {
			b[c++] = d[i];
		}
	}

	// send data frame
	if(dwrite(tty, b, c) < c) return(0);

	return(length);
}

ssize_t rabbit_sread(int tty, void *data, size_t length) {
	unsigned char *d = data;
	ssize_t r, c;
#ifdef DEBUG_COMM
	fprintf(stderr, "reading %d bytes\n", count);
#endif
	for(c = 0; c < length; c++, d++) {
		for(r = 0; !r;) if((r = read(tty, d, 1)) < 0) return(r);
		dread_count += r;

		if(*d == TC_FRAMING_ESC) for(r = 0; !r;) {
			if((r = read(tty, d, 1)) < 0) return(r);
			*d ^= 0x20;
		}
	}
#ifdef DEBUG_COMM
  fprintf(stderr, "read %d bytes\n", c);
#endif
#ifdef DEBUG_IO
  write(1, buf, c);
#endif
	return(c);
}

