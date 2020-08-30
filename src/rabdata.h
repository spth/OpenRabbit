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

#ifndef _RABDATA_H
#define _RABDATA_H

#define rabbit_load(var, src) rabbit_getvar(&var, src, sizeof(var))

uint16 rabbit_csum(uint16 initial, uint8 *data, uint16 length);

void rabbit_getvar(void *var, void **src, size_t sz);

void rabbit_parse_info(_TCSystemInfoProbe *info, void *src);
void rabbit_parse_registers(struct __dkregisters *regs, void *src);

void rabbit_show_registers(struct __dkregisters *regs);

#endif // _RABDATA_H

