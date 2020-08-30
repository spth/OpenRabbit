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

#ifndef _MYIO_C
#define _MYIO_C

#include <unistd.h>

ssize_t dwrite(int fd, const void *buf, size_t count);
ssize_t dread(int fd, void *buf, size_t count);
char *load(char *pb, char *file, int *sz);
char tty_setbaud(int tty, int baud);
void dtiming(int *rs, int *ws);

extern unsigned long dwrite_count;
extern unsigned long dwrite_time;
extern unsigned long dread_count;
extern unsigned long dread_time;

#endif // _MYIO_C
