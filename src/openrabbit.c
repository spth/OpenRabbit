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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ncurses.h>
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
#include <regex.h>

#include "myio.h"
#include "mytypes.h"
#include "bios/tc_defs.lib"
#include "bios/dkcore.lib"
#include "rabdata.h"
#include "rabio.h"
#include "rabbit.h"
#include "rabmap.h"

WINDOW *win_watch;
WINDOW *win_stack;
WINDOW *win_regs;
WINDOW *win_file;
WINDOW *win_stdio;

typedef struct {
	struct {
		int row, col;
	} start, end;
} coords;

void sprintw(char *fmt, ...);

void sprintw(char *fmt, ...) {
	va_list a;
	int height,width;
	getmaxyx(win_stdio, height, width);
	scroll(win_stdio);
	wmove(win_stdio,height-2, 1);
	va_start(a, fmt);
	vwprintw(win_stdio, fmt, a);
	box(win_stdio,0,0);
	wrefresh(win_stdio);
}

void screen_create(void) {
	int height, width;

	// get dimension
	getmaxyx(stdscr, height, width);

	// write some top row
	attron(COLOR_PAIR(1));
	mvprintw(0, 0, " OpenRabbit %s  F6=Auto  F7=Trace  F8=Step  F9=Run  F10=Stop", VERSION);
	refresh();

	// create dimensions
	int x1 = 0;
	int w2 = 15;
	int w1 = width-w2;
	int x2 = x1+w1;
	int y1 = 1;
	int h1 = 0.4*height;
	int y2 = y1+h1;
	int h2 = 0.4*height;
	int y3 = y2+h2;
	int h3 = height-y3;
	int y4 = y1;
	int h4 = 17;
	int y5 = y4+h4;
	int h5 = height-y5;

	// create windows
	win_file  = newwin(h1, w1, y1, x1);
	win_stdio = newwin(h2, w1, y2, x1);
	win_watch = newwin(h3, w1, y3, x1);
	win_regs  = newwin(h4, w2, y4, x2);
	win_stack = newwin(h5, w2, y5, x2);

	// box 'em
	box(win_regs,  0, 0);
	box(win_file,  0, 0);
	box(win_stdio, 0, 0);
	box(win_watch, 0, 0);
	box(win_stack, 0, 0);

	// scrolling ok
	scrollok(win_stdio, true);

	// draw 'em
	wrefresh(win_regs);
	wrefresh(win_file);
	wrefresh(win_stdio);
	wrefresh(win_watch);
	wrefresh(win_stack);
}

void screen_init(void) {
	// initialize screen
	initscr();
	raw();
	keypad(stdscr, true);
	noecho();

	// setup colors
	start_color();
	init_pair(1, COLOR_CYAN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);

	// create windows
	screen_create();
}

void rabbit_regs(struct __dkregisters *regs) {
	int y = 1, x = 1;
	mvwprintw(win_regs, y++, x, " af' %04x", regs->_afPrime);
	mvwprintw(win_regs, y++, x, " bc' %04x", regs->_bcPrime);
	mvwprintw(win_regs, y++, x, " de' %04x", regs->_dePrime);
	mvwprintw(win_regs, y++, x, " hl' %04x", regs->_hlPrime);
	mvwprintw(win_regs, y++, x, " af  %04x", regs->_af);
	mvwprintw(win_regs, y++, x, " bc  %04x", regs->_bc);
	mvwprintw(win_regs, y++, x, " de  %04x", regs->_de);
	mvwprintw(win_regs, y++, x, " hl  %04x", regs->_hl);
	mvwprintw(win_regs, y++, x, " ix  %04x", regs->_ix);
	mvwprintw(win_regs, y++, x, " iy  %04x", regs->_iy);
	mvwprintw(win_regs, y++, x, " sp  %04x", regs->_sp);
	mvwprintw(win_regs, y++, x, " xpc %04x", regs->_xpc);
	mvwprintw(win_regs, y++, x, " ip  %04x", regs->_ip);
	mvwprintw(win_regs, y++, x, " pc  %04x", regs->_pc);
	wrefresh(win_regs);
}

void rabbit_stack(struct __dkregisters *regs, void *s, int len) {
	uint16 *stack = s;
	int o, i;
	for(o = 0, i = 0; o < len; o+=2, i++)
		mvwprintw(win_stack, i+1, 1, " %04x: %04x", regs->_sp+o, stack[i]);
	wrefresh(win_stack);
}

#define FILES_MAX 256

struct {
	char *filename;
	char *pb;
	int sz;
	char **row;
	struct {
		int first;
		int total;
	} lines;
} files[FILES_MAX];

void fileinit(void) {
	int i;

	// empty file slots
	for(i = 0; i < FILES_MAX; i++) {
		files[i].filename = NULL;
		files[i].row = NULL;
		files[i].pb = NULL;
	}
}

void fileclean(void) {
	int i;

	// clean up file slots
	for(i = 0; i < FILES_MAX; i++) {
		if(files[i].filename != NULL) free(files[i].filename);
		if(files[i].row != NULL) free(files[i].row);
		if(files[i].pb != NULL) free(files[i].pb);
	}
}

char fileshow(int index, coords *c) {
	int height, width;
	char *b = NULL;
	int r, i, e;
	int a1, a2;

	// empty the screen
	werase(win_file);
	box(win_file,0,0);

	// get dimension
	getmaxyx(win_file, height, width);
	if((b = malloc(width)) == NULL) {
		perror("malloc(width)");
		return(0);
	}

	// adjust dimension
	b[--width] = '\0';
	height-=2;

	// check line
	r = files[index].lines.first;
	if(c->start.row < r) r = c->start.row;
	if(c->start.row-height+1 >= r) r = c->start.row-height+1;
	files[index].lines.first = r;

	// process the file
	for(i = 1; i <= height; i++, r++) {
		if(r >= files[index].lines.total) break;
		strncpy(b, files[index].row[r], width-1);
		for(a1 = 0, a2 = strlen(b); a1 < a2; a1++) if(b[a1] == '\t') b[a1] = ' ';
		mvwprintw(win_file, i, 1, b);
		if(c->start.row == r) {
			if(c->end.col < c->start.col || c->end.row > c->start.row || c->end.col > strlen(b))
				e = strlen(b);
			else
				e = c->end.col;
			wattron(win_file, COLOR_PAIR(2));
			memmove(b, b+c->start.col, e-c->start.col);
			b[e-c->start.col] = '\0';
			mvwprintw(win_file, i, 1+c->start.col, b);
			wattroff(win_file, COLOR_PAIR(2));
		}
	}

	// draw it
	wrefresh(win_file);
	free(b);
	return(0);
}

int fileload(char *filename, char *drive, char *mount) {
	char newname[1024];
	char *n;
	int c, i;

	// check if we need to modify it
	if(strncasecmp(filename, drive, strlen(drive)) == 0) {
		strcpy(newname, mount);
		strncat(newname, filename+strlen(drive), sizeof(newname)-strlen(newname)-1);
	} else {
		strncpy(newname, filename, sizeof(newname)-1);
	}
	newname[sizeof(newname)-1] = 0;

	// search empty file slot, or already loaded one
	for(i = 0; i < FILES_MAX; i++) {
		if(files[i].filename == NULL) break;
		if(strcmp(files[i].filename, newname) == 0) return(i);
	}

	// check maximum
	if(i == FILES_MAX) {
		fprintf(stderr, "error: out of memory (%s)\n", newname);
		return(-1);
	}

	// setup slot
	files[i].filename = malloc(strlen(newname)+1);
	strcpy(files[i].filename, newname);
	files[i].lines.first = 0;

	// load the file
	if((files[i].pb = load(files[i].pb, newname, &files[i].sz)) == NULL) return(-1);

	// count lines in file
	for(n = files[i].pb, c = 0;; n++, c++) {
		n = memchr(n, '\n', files[i].sz-(n-files[i].pb));
		if(n == NULL) break;
	}

	// store line count
	files[i].lines.total = c;

	// allocate memory	(one extra to find last \n)
	if((files[i].row = calloc(c+1, sizeof(*files[i].row))) == NULL) {
		perror("calloc()");
		return(-1);
	}

	// store row references
	for(n = files[i].pb, c = 0;; n++, c++) {
		files[i].row[c] = n;
		n = memchr(n, '\n', files[i].sz-(n-files[i].pb));
		if(n == NULL) break;
		*n = '\0';
	}

	return(i);
}

char *regpart(char *p, regmatch_t *r) {
	p[r->rm_eo] = '\0';
	return(p+r->rm_so);
}

#define RABBIT_BRKS_MAX 16384

struct {
	int file;
	int xpc;
	unsigned int pc;
	coords c;
} rabbit_brks[RABBIT_BRKS_MAX];

int rabbit_brks_count;

char rabbit_brk_search(struct __dkregisters *regs) {
	int i;

	// search breakpoint
	for(i = 0; i < rabbit_brks_count; i++) {
		if(rabbit_brks[i].pc == regs->_pc && rabbit_brks[i].xpc == regs->_xpc) break;
	}

	// search breakpoint
	if(i == rabbit_brks_count) for(i = 0; i < rabbit_brks_count; i++) {
		if(rabbit_brks[i].pc == regs->_pc && rabbit_brks[i].xpc == -1) break;
	}

	// check if we didn't found
	if(i == rabbit_brks_count) {
		sprintw("breakpoint (%02x)%04x not found in sources...\n", regs->_xpc, regs->_pc);
		return(0);
	}

	// showfile
	fileshow(rabbit_brks[i].file, &rabbit_brks[i].c);
	return(1);
}

char rabbit_brk_load(char *filename, char *drive, char *mount) {
	char *pb = NULL;
	regmatch_t result[10];
	char *p, *pn, *t;
	regex_t regex;
	int err = 0;
	int i, a, b;
	int sz, c;

	// F:\DCRABBIT_8.51\RPROJECTS\LIB\BOB3.LIB([160,0],[166,0]@[160,0])->**:3940

	// assemble regular expression
	memset(&regex, 0, sizeof(regex_t));
	if(regcomp(&regex, "^(.*)\\(\\[([0-9]*),([0-9]*)\\],\\[([0-9]*),([0-9]*)\\]@\\[([0-9]*),([0-9]*)\\]\\)\\->(.*):([0-9a-fA-F]*)\r?$", REG_EXTENDED) != 0) {
		fprintf(stderr, "regcomp() failed\n");
    regfree(&regex);
   	return 0;
	}

	// load the file
	if((pb = load(pb, filename, &sz)) == NULL) return(0);
	
	// parse the file
	for(p = pb, c = 0, i = 0;; p=pn+1, c++) {
		pn = memchr(p, '\n', sz-(p-pb));
		if(pn == NULL) break; *pn = '\0';

		if(regexec(&regex, p, 10, result, 0) == 0) {
			// check memory
			if(i >= RABBIT_BRKS_MAX) {
				fprintf(stderr, "error: out of break memory\n");
				goto rabbit_brk_load_abort;
			}

			// convert filename
			t = regpart(p, &result[1]);
			for(a = 0, b = strlen(t); a < b; a++) if(t[a] == '\\') t[a] = '/';

			// load the file
			if((rabbit_brks[i].file = fileload(t, drive, mount)) < 0) goto rabbit_brk_load_abort;

			// store xpc and pc
			t = regpart(p, &result[8]);
			if(t[0] == '*' && t[1] == '*')
				rabbit_brks[i].xpc = -1;
			else
				rabbit_brks[i].xpc = strtol(t, NULL, 16);
			rabbit_brks[i].pc = strtol(regpart(p, &result[9]), NULL, 16);

			// store position
			rabbit_brks[i].c.end.row = atoi(regpart(p, &result[4]))-1;
			rabbit_brks[i].c.end.col = atoi(regpart(p, &result[5]));
			if(rabbit_brks[i].c.end.col > 0) rabbit_brks[i].c.end.col--;
			rabbit_brks[i].c.start.row = atoi(regpart(p, &result[6]))-1;
			rabbit_brks[i].c.start.col = atoi(regpart(p, &result[7]));
			if(rabbit_brks[i].c.start.col > 0) rabbit_brks[i].c.start.col--;

			// next
			i++;
		} else {
			fprintf(stderr, "skipped breakpoint: %s\n", p);
		}
	}

	// store count
	fprintf(stderr, "loaded %d breakpoints\n", i);
	rabbit_brks_count = i;

#if 0
	// show breakpoints
	for(i = 0; i < rabbit_brks_count; i++)
		fprintf(stderr, "%s %dx%d,%dx%d @ %02x:%04x\n", files[rabbit_brks[i].file].filename, rabbit_brks[i].c.start.row,
			rabbit_brks[i].c.start.col, rabbit_brks[i].c.end.row, rabbit_brks[i].c.end.col, rabbit_brks[i].xpc, rabbit_brks[i].pc);
#endif
	
	// success
	err = 1;
rabbit_brk_load_abort:
  regfree(&regex);
	free(pb);
	return(err);
}

int main(int argc, char **argv) {
	struct __dkregisters regs;
	struct timeval timeout;
	unsigned char b[1024];
	_TC_PacketHeader tcph;
	char autorun = 0;
	_TCSystemREAD tcsr;
	fd_set readfs;
	char rfu = 0;
	int err = 4;
	int rs, ws;
	char stop;
	char arg;
	int key;
	int tty;
	int i,c;

	// check argument count
  if(argc <= 7) {
	  fprintf(stderr, "Usage: %s <coldload.bin> <pilot.bin> <project.bin> <project.brk> <drive> <mount> <device>\n", argv[0]);
    return(1);
	}

	// are we the just the rfu?
	if(strcmp(argv[0]+strlen(argv[0])-strlen("rfu"), "rfu") == 0) rfu = 1;

	// make connection
	if((tty = rabbit_open(argv[7])) < 0) return(2);

	if(!rfu) {
		// initialize file slots
		fileinit();

		// load break file if we're not rfu
		if(!rabbit_brk_load(argv[4], argv[5], argv[6])) return(3);
	}

	// program the damn thing
	if(! rabbit_program(tty, argv[1], argv[2], argv[3])) {
		close(tty);
		return(3);
	}

	// stop here if we're rfu
	if(rfu) return(0);

	// start debug
	if(! rabbit_debug(tty)) {
		close(tty);
		return(4);
	}

	// initialize screen
	screen_init();

	// stop once, to init
	if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_STOPPROGRAM, 0, NULL)) goto main_abort;

	for(c = 0, stop = 0; !stop;) {
		// update bps	FIXME: find a good place for this :)
		if(0) {
			move(0, 60);
			dtiming(&rs, &ws);
			printw("BPS: in=%d, out=%d        \n", rs, ws);
			refresh();
		}

		// select
		FD_SET(0, &readfs);
		FD_SET(tty, &readfs);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;
		i = select(tty+1, &readfs, NULL, NULL, &timeout);

		// nothing found?
		if(!i) {
			if(++c > 5) {
				// ping the damn thing
				sprintw("ping?\n");
				if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_NOOP, 0, NULL)) goto main_abort;
				c = 0;
			}
			continue;
		}

		// check for a key
		if(FD_ISSET(0, &readfs)) {
			// get the key
			key = getch();

			// abort autorun?
			if(autorun) {
				if(key == KEY_F(6))
					autorun = 0;
				else
					sprintw("only F6 is valid now, pressed %d %02x\n", key, key);
				continue;
			}

			// process key
			switch(key) {
				case(KEY_F(6)):
					autorun = 1;
					// yes do a singlestep
				case(KEY_F(7)):
					arg = DKF_STEP_INTO+9;	// why add 9 ???
					if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SINGLESTEP, sizeof(arg), &arg)) goto main_abort;
					break;
				case(KEY_F(8)):
					arg = DKF_STEP_OVER+9;
					if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SINGLESTEP, sizeof(arg), &arg)) goto main_abort;
					break;
				case(KEY_F(9)):
					if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_RUNPROGRAM, 0, NULL)) goto main_abort;
					break;
				case(KEY_F(10)):
					if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_STOPPROGRAM, 0, NULL)) goto main_abort;
					break;
				case('r'):
					if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_REGDATA, 0, NULL)) goto main_abort;
					break;
				case('q'):
					stop = 1;
					break;
				default:
					sprintw("unknown key: %d %02x\n", key, key);
			}
		}

		// data?
		if(FD_ISSET(tty, &readfs)) {
			// listen to her
			if(!rabbit_poll(tty, &tcph, sizeof(b), b)) goto main_abort;

			// debug?
			if(tcph.type == TC_TYPE_DEBUG) switch(tcph.subtype) {
				case(TC_DEBUG_STDIO):
					b[tcph.length] = '\0';
					sprintw(b);
					break;
				case(TC_DEBUG_REGDATA):
					rabbit_parse_registers(&regs, b);
					tcsr.type = TC_SYSREAD_NOXPC;
					tcsr.length = 0x20;
					tcsr.address.logical.offset = regs._sp;
					tcsr.address.logical.xpc = 0;
					memcpy(b,&tcsr,8);
					memcpy(b+1,b+2,6);
				  if(!rabbit_write(tty, TC_TYPE_SYSTEM, TC_SYSTEM_READ, TC_SYSTEM_READ_HEADERSIZE, b)) goto main_abort;
					rabbit_regs(&regs);
					break;
				case(TC_DEBUG_RUNPROGRAM):
					sprintw("program is running\n");
					break;
				case(TC_DEBUG_STOPPROGRAM):
					sprintw("program stopped\n");
					break;
				case(TC_DEBUG_ATBREAKPOINT):
					rabbit_brk_search(&regs);
					break;
				case(TC_DEBUG_SINGLESTEP):
				/*
					if(autorun) {
						arg = DKF_STEP_INTO+9;
						if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SINGLESTEP, sizeof(arg), &arg)) goto main_abort;
					}
					*/
					break;
				default:
					sprintw("unknown debug packet received 0x%02x\n", tcph.subtype); 
			} else

			// system?
			if(tcph.type == TC_TYPE_SYSTEM) switch(tcph.subtype) {
				case(TC_SYSTEM_NOOP):
					sprintw("pong!\n");
					break;
				case(TC_SYSTEM_READ):
					if(tcph.length > 6) rabbit_stack(&regs, b+6, tcph.length-6);
					// should the next 4 lines be here? :)
					if(autorun) {
						arg = DKF_STEP_INTO+9;
						if(!rabbit_write(tty, TC_TYPE_DEBUG, TC_DEBUG_SINGLESTEP, sizeof(arg), &arg)) goto main_abort;
					}
					break;
				default:
					sprintw("unknown system packet received 0x%02x\n", tcph.subtype); 
					break;
			} else

			sprintw("unknown type received 0x%02x (0x%02x)\n", tcph.type, tcph.subtype);
		}
	}

	// success
	err = 0;
main_abort:
	fileclean();
	endwin();
	close(tty);
	return(err);
}

