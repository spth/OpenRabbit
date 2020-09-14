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

int rabbit_reset(int tty);
int rabbit_open(const char *device);
char rabbit_write(int tty, uint8 type, uint8 subtype, uint16 length, void *data);
char rabbit_poll(int tty, _TC_PacketHeader *tcph, uint16 length, void *data);
char rabbit_read(int tty, uint8 type, uint8 subtype, uint16 length, void *data);
int rabbit_coldload(int tty, const char *file);
int rabbit_pilot(int tty, const char *pfile);
int rabbit_upload(int tty, const char *project);
char rabbit_boot(int tty, char *coldload, char *pilot, char *project);
char rabbit_debug(int tty);
int rabbit_program(int tty, char *coldload, char *pilot, char *project);

#endif

