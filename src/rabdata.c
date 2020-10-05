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

#include <string.h>
#include <stdio.h>

#include "rabdata.h"

#include "bios/dkcore.lib"

extern int verbose;

uint16_t rabbit_csum(uint16_t initial, uint8_t *data, uint16_t length) {
	uint16_t a, b;
	int i;

	// split in a and b
	a = initial >> 8;
	b = initial & 0xff;

	// go along data
	for(i = 0; i < length; i++) {
		a += data[i];
		a = (a & 0xff) + (a >> 8);
		b += a;
		b = (b & 0xff) + (b >> 8);
	}

	// merge a and b
	return(b | (a << 8));
}

void rabbit_getvar(void *var, const void **src, size_t sz) {
	memcpy(var, *src, sz);
	*(const unsigned char **)(src) += sz;
}

void rabbit_parse_info(_TCSystemInfoProbe *info, const void *src) {
	const char *b = src;

	// system info probe header
	rabbit_load(info->FlashID, &src);
	rabbit_load(info->RamSize, &src);
	rabbit_load(info->div19200, &src);
	rabbit_load(info->CpuID, &src);

	// system info probe idblock
	rabbit_load(info->IDBlock.tableVersion, &src);
	rabbit_load(info->IDBlock.productID, &src);
	rabbit_load(info->IDBlock.vendorID, &src);
	rabbit_load(info->IDBlock.timestamp, &src);
	rabbit_load(info->IDBlock.flashID, &src);
	rabbit_load(info->IDBlock.flashType, &src);
	rabbit_load(info->IDBlock.flashSize, &src);
	rabbit_load(info->IDBlock.sectorSize, &src);
	rabbit_load(info->IDBlock.numSectors, &src);
	rabbit_load(info->IDBlock.flashSpeed, &src);
	rabbit_load(info->IDBlock.flash2ID, &src);
	rabbit_load(info->IDBlock.flash2Type, &src);
	rabbit_load(info->IDBlock.flash2Size, &src);
	rabbit_load(info->IDBlock.sector2Size, &src);
	rabbit_load(info->IDBlock.num2Sectors, &src);
	rabbit_load(info->IDBlock.flash2Speed, &src);
	rabbit_load(info->IDBlock.ramID, &src);
	rabbit_load(info->IDBlock.ramSize, &src);
	rabbit_load(info->IDBlock.ramSpeed, &src);
	rabbit_load(info->IDBlock.cpuID, &src);
	rabbit_load(info->IDBlock.crystalFreq, &src);
	rabbit_load(info->IDBlock.macAddr, &src);
	rabbit_load(info->IDBlock.serialNumber, &src);
	rabbit_load(info->IDBlock.productName, &src);
	rabbit_load(info->IDBlock.reserved, &src);
	rabbit_load(info->IDBlock.idBlockSize, &src);
	rabbit_load(info->IDBlock.userBlockSize, &src);
	rabbit_load(info->IDBlock.userBlockLoc, &src);
	rabbit_load(info->IDBlock.idBlockCRC, &src);
	rabbit_load(info->IDBlock.marker, &src);

	if(verbose > 1)
		fprintf(stderr, "parsed %td bytes\n", (const char *)src - b);
}

void rabbit_parse_registers(struct __dkregisters *regs, const void *src) {
	rabbit_getvar(&regs->_sp, &src, 2);
	rabbit_getvar(&regs->_xpc, &src, 2);
	rabbit_getvar(&regs->_afPrime, &src, 2);
	rabbit_getvar(&regs->_bcPrime, &src, 2);
	rabbit_getvar(&regs->_dePrime, &src, 2);
	rabbit_getvar(&regs->_hlPrime, &src, 2);
	rabbit_getvar(&regs->_af, &src, 2);
	rabbit_getvar(&regs->_bc, &src, 2);
	rabbit_getvar(&regs->_de, &src, 2);
	rabbit_getvar(&regs->_hl, &src, 2);
	rabbit_getvar(&regs->_ix, &src, 2);
	rabbit_getvar(&regs->_iy, &src, 2);
	rabbit_getvar(&regs->_ip, &src, 1);
	rabbit_getvar(&regs->_pc, &src, 2);
}

void rabbit_show_registers(const struct __dkregisters *regs) {
	fprintf(stderr, "registers:\n");

	fprintf(stderr, "  sp  %6d 0x%04x\n", regs->_sp, regs->_sp);
	fprintf(stderr, "  xpc %6d 0x%04x\n", regs->_xpc, regs->_xpc);
	fprintf(stderr, "  af' %6d 0x%04x\n", regs->_afPrime, regs->_afPrime);
	fprintf(stderr, "  bc' %6d 0x%04x\n", regs->_bcPrime, regs->_bcPrime);
	fprintf(stderr, "  de' %6d 0x%04x\n", regs->_dePrime, regs->_dePrime);
	fprintf(stderr, "  hl' %6d 0x%04x\n", regs->_hlPrime, regs->_hlPrime);
	fprintf(stderr, "  af  %6d 0x%04x\n", regs->_af, regs->_af);
	fprintf(stderr, "  bc  %6d 0x%04x\n", regs->_bc, regs->_bc);
	fprintf(stderr, "  de  %6d 0x%04x\n", regs->_de, regs->_de);
	fprintf(stderr, "  hl  %6d 0x%04x\n", regs->_hl, regs->_hl);
	fprintf(stderr, "  ix  %6d 0x%04x\n", regs->_ix, regs->_ix);
	fprintf(stderr, "  iy  %6d 0x%04x\n", regs->_iy, regs->_iy);
	fprintf(stderr, "  ip  %6d 0x%04x\n", regs->_ip, regs->_ip);
	fprintf(stderr, "  pc  %6d 0x%04x\n", regs->_pc, regs->_pc);
}

const char *rabbit_cpuname(uint16_t cpuid){
	switch(cpuid) {
	case 0x0000:
		return("Rabbit 2000");
	case 0x0001:
		return("Rabbit 2000A");
	case 0x0002:
		return("Rabbit 2000B");
	case 0x0003:
		return("Rabbit 2000C");
	case 0x0100:
		return("Rabbit 3000");
	case 0x0101:
		return("Rabbit 3000A");
	case 0x0200:
		return("Rabbit 4000");
	case 0x0300:
		return("Rabbit 5000");
	case 0x0400:
		return("Rabbit 6000");
	default:
		return("Unknown processor");
	}
}

