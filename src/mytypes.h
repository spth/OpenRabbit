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

#ifndef _MYTYPES_H
#define _MYTYPES_H

typedef unsigned char uint8;
typedef unsigned short int uint16;
typedef short int int16;
typedef unsigned int uint32;
typedef unsigned int faraddr_t;

typedef struct {
	int16	tableVersion;		// version number for this table layout
	int16	productID;			// Z-World part #
	int16	vendorID;			// 1 = Z-World
	char	timestamp[7];		//	YY/M/D H:M:S
	uint32 flashID;				// Z-World part #
	int16	flashType;			// Write method
	int16	flashSize;			// in 1000h pages
	int16	sectorSize;			// size of flash sector in bytes
	int16	numSectors;			// number of sectors
	int16	flashSpeed;			// in nanoseconds
	uint32 flash2ID;			// Z-World part #, 2nd flash
	int16	flash2Type;			// Write method, 2nd flash
	int16	flash2Size;			// in 1000h pages, 2nd flash
	int16	sector2Size;		// size of 2nd flash's sectors in bytes
	int16	num2Sectors;		// number of sectors
	int16	flash2Speed;		// in nanoseconds, 2nd flash
	uint32 ramID;				// Z-World part #
	int16	ramSize;				// in 1000h pages
	int16	ramSpeed;			// in nanoseconds
	int16	cpuID;				// CPU type identification
	uint32 crystalFreq;		// in Hertz
	char	macAddr[6];			// Media Access Control (MAC) address
	char	serialNumber[24];	// device serial number
	char	productName[30];	// null-terminated string
	char	reserved[1];		// reserved for later use -- can grow in size
	uint32 idBlockSize;		// size of the SysIDBlock struct
	uint16 userBlockSize;	// size of user block (directly below ID block)
	uint16 userBlockLoc;	// offset of start of user block from start of ID block
	int16 idBlockCRC;			// CRC of this block (with this field set to zero)
	char	marker[6];			// should be 0x55 0xAA 0x55 0xAA 0x55 0xAA 0x55 0xAA
} SysIDBlockType;

#endif // _MYTYPES_H

