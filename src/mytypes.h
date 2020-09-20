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

#ifndef OPENRABBIT_MYTYPES_H
#define OPENRABBIT_MYTYPES_H

#include <stdint.h>

typedef uint32_t faraddr_t;

typedef struct {
	int16_t	tableVersion;		// version number for this table layout
	int16_t	productID;			// Z-World part #
	int16_t	vendorID;			// 1 = Z-World
	char	timestamp[7];		//	YY/M/D H:M:S
	uint32_t flashID;				// Z-World part #
	int16_t	flashType;			// Write method
	int16_t	flashSize;			// in 1000h pages
	int16_t	sectorSize;			// size of flash sector in bytes
	int16_t	numSectors;			// number of sectors
	int16_t	flashSpeed;			// in nanoseconds
	uint32_t flash2ID;			// Z-World part #, 2nd flash
	int16_t	flash2Type;			// Write method, 2nd flash
	int16_t	flash2Size;			// in 1000h pages, 2nd flash
	int16_t	sector2Size;		// size of 2nd flash's sectors in bytes
	int16_t	num2Sectors;		// number of sectors
	int16_t	flash2Speed;		// in nanoseconds, 2nd flash
	uint32_t ramID;				// Z-World part #
	int16_t	ramSize;				// in 1000h pages
	int16_t	ramSpeed;			// in nanoseconds
	int16_t	cpuID;				// CPU type identification
	uint32_t crystalFreq;		// in Hertz
	char	macAddr[6];			// Media Access Control (MAC) address
	char	serialNumber[24];	// device serial number
	char	productName[30];	// null-terminated string
	char	reserved[1];		// reserved for later use -- can grow in size
	uint32_t idBlockSize;		// size of the SysIDBlock struct
	uint16_t userBlockSize;	// size of user block (directly below ID block)
	uint16_t userBlockLoc;	// offset of start of user block from start of ID block
	int16_t idBlockCRC;			// CRC of this block (with this field set to zero)
	char	marker[6];			// should be 0x55 0xAA 0x55 0xAA 0x55 0xAA 0x55 0xAA
} SysIDBlockType;

#endif // _MYTYPES_H

