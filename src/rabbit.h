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

#ifndef OPENRABBIT_RABBIT_H
#define OPENRABBIT_RABBIT_H 1

#include <stdint.h>

#include "mytypes.h"
#include "bios/tc_defs.lib"

int rabbit_reset(int tty);
int rabbit_open(const char *device);
char rabbit_write(int tty, uint8_t type, uint8_t subtype, uint16_t length, void *data);
int rabbit_poll(int tty, _TC_PacketHeader *tcph, uint16_t length, void *data);
int rabbit_read(int tty, uint8_t type, uint8_t subtype, uint16_t length, void *data);
int rabbit_coldload(int tty, const char *file);
int rabbit_pilot(int tty, const char *pfile, bool *dc8pilot);
int rabbit_upload(int tty, const char *project, bool dc8pilot);
char rabbit_boot(int tty, char *coldload, char *pilot, char *project);
char rabbit_debug(int tty);

// Load program into Rabbit. Use tty for serial device, coldload for initial loder filename, pilot for secondary loader filename, project for user program filename, dc8pilot indicates that the secondary loader is Dynamic C 8-style instead of Dynamic C 9.
int rabbit_program(int tty, const char *coldload, const char *pilot, const char *project, bool *dc8pilot);

// Start program in flash.
int rabbit_start(int tty);

extern unsigned int verbose;
extern unsigned int slow;

#endif

