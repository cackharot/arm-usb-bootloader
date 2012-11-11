/*****************************************************************************\
*              efs - General purpose Embedded Filesystem library              *
*          --------------------- -----------------------------------          *
*                                                                             *
* Filename : sd.c                                                             *
* Revision : Initial developement                                             *
* Description : This file contains the functions needed to use efs for        *
*               accessing files on an SD-card.                                *
*                                                                             *
* This library is free software; you can redistribute it and/or               *
* modify it under the terms of the GNU Lesser General Public                  *
* License as published by the Free Software Foundation; either                *
* version 2.1 of the License, or (at your option) any later version.          *
*                                                                             *
* This library is distributed in the hope that it will be useful,             *
* but WITHOUT ANY WARRANTY; without even the implied warranty of              *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU           *
* Lesser General Public License for more details.                             *
*                                                                             *
*                                                    (c)2005 Michael De Nil   *
*                                                    (c)2005 Lennart Yseboodt *
\*****************************************************************************/

#include <string.h>	// memcpy
#include "type.h"

#include "sys.h"
#include "svc.h"
#include "iap.h"
#include "msc_scsi.h"
#include "blockdev.h"

#include "usbdebug.h"

#define BOOTSECT_START		0
#define FAT_START			BLOCKSIZE
#define ROOTDIR_START		(3 * BLOCKSIZE)
#define DATA_START			(4 * BLOCKSIZE)

#define AUTORUN_START		(5 * BLOCKSIZE)
#define LOGO_START			(6 * BLOCKSIZE)

#define RAMDISK_SIZE		(14 * BLOCKSIZE)

// starting address of user flash space
// Computed in BlockDevInit based on USER_START_SECTOR
unsigned long USER_FLASH_START=0;
unsigned long USER_FLASH_SIZE=0;
unsigned long MSC_MemorySize=0;			// total size of device = size of ramdisk + size of flash
unsigned long MSC_BlockCount=0;			// total number of blocks

unsigned char RamDisk[RAMDISK_SIZE];

const unsigned char BootSect[] = {			// offset,		length,	description
	0xEB, 0x3C, 0x90,						// 0x00,		3,		Jump Instruction
	'M', 'S', 'W', 'I', 'N', '4', '.', '1', // 0x03,		8,		OEM Name (padded with spaces)
	LE_WORD(BLOCKSIZE),						// 0x0b,		2,		Bytes per sector (AKA block)
	1,										// 0x0d,		1,		Sectors (blocks) per cluster
	LE_WORD(1),								// 0x0e,		2,		Reserved sector count. Should be 1 for fat16
	2,										// 0x10,		1,		Number of FATS. Should be 2
	LE_WORD(16),							// 0x11,		2,		Maximum number of root directory entries
	LE_WORD(0),								// 0x13,		2,		Total Sectors. Needs to be set during intialization!!
	0xF8,									// 0x15,		1,		Media Descriptor: fixed disk
	LE_WORD(1),								// 0x16,		2,		Sectors per file allocation table
	LE_WORD(1),								// 0x18,		2,		Sectors per track
	LE_WORD(1),								// 0x1a,		2,		Number of heads
	0, 0, 0, 0,								// 0x1c,		4,		Count of hidden sectors preceding the partition that contains this FAT volume. This field should always be zero on media that are not partitioned.
	0, 0, 0, 0,								// 0x20,		4,		Total sectors if greater than 65535. Otherwise should be 0
	0x80,									// 0x24,		1,		Physical drive number (0x00 for removable media, 0x80 for hard disks)
	0,										// 0x25,		1,		Reserved ("current head"), In Windows NT bit 0 is a dirty flag to request chkdsk at boot time. bit 1 requests surface scan too.[30]
	0x29,									// 0x26,		1,		Extended boot signature. (Should be 0x29[29] to indicate that an Extended BIOS Parameter Block with the following 3 entries exists. Can be 0x28 on some OS/2 1.x and DOS disks indicating an earlier form of the EBPB format with only the serial number following.)
	0xDF, 0xD7, 0x20, 0x85,					// 0x27,		4,		ID (serial number) randomly generated
	'I', 'N', 'T', 'E', 'R', 'L', 'O','G', 'I', 'X', 0x20,	// 0x2b,		11,		Volume Label, padded with blanks (0x20).
	'F', 'A', 'T', '1', '2', ' ', ' ', ' ',	// 0x36,		8,		FAT file system type, padded with blanks (0x20), e.g.: "FAT12   ", "FAT16   ". This is not meant to be used to determine drive type, however, some utilities use it in this way.
};

// The first two entries in a FAT store special values: 
// The first entry contains a copy of the media descriptor 
// (from boot sector, offset 0x15). The remaining 8 bits (if FAT16),
// or 20 bits (if FAT32) of this entry are 1.
// The second entry stores the end-of-cluster-chain marker.
// The high order two bits of this entry are sometimes,
// in the case of FAT16 and FAT32, used for dirty volume 
// management: high order bit 1: last shutdown was clean;
// next highest bit 1: during the previous mount no disk I/O errors were detected.
/*const unsigned char FAT[] = {
	0x8f, 0xff, 0xff,		// entries 0 and 1
	0xff, 0x0f, 0x00		// entries 2 and 3
};*/
const unsigned char FAT[] = {
	0x8f,0xff,0x00,0x00,0xf0,0xff,0x05,0x60,0x00,0x07,
	0x80,0x00,0x09,0xA0,0x00,0x0B,0xC0,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0x00
};


// consists of volume label and entry for EMPTY file FIRMWARE.BIN
const unsigned char RootDir[] = {
 'I', 'N', 'T', 'E', 'R', 'L', 'O', 'G', 'I', 'X', ' ',0x28,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
 'A', 'U', 'T', 'O', 'R', 'U', 'N', ' ', 'I', 'N', 'F',0x07,0x00,0x64,0x02,0x82,0x7e,0x40,0x7e,0x40,0x00,0x00,0x02,0x82,0x7e,0x40,0x03,0x00,0x41,0x00,0x00,0x00,
 'L', 'O', 'G', 'O', ' ', ' ', ' ', ' ', 'I', 'C', 'O',0x07,0x00,0x00,0xb6,0x84,0x7e,0x40,0x7e,0x40,0x00,0x00,0x02,0x82,0x7e,0x40,0x04,0x00,0xbe,0x10,0x00,0x00,
 'F', 'I', 'R', 'M', 'W', 'A', 'R', 'E', 'B', 'I', 'N',0x24,0x00,0x64,0x38,0x87,0x7e,0x40,0x7e,0x40,0x00,0x00,0x38,0x87,0x7e,0x40,0x0D,0x00,0x00,0x80,0x07,0x00,
 };

const unsigned char autoruninf[] = {
	'[','A','u','t','o','R','u','n',']',0x0a,0x0d,'L','a','b','e','l','=',
	'I','n','t','e','r','l','o','g','i','c','x',' ','L','P','C','2','1','4','8',
	' ','B','o','a','r','d',' ','v','1','.','0',' ',0x0a,0x0d,
	'I','c','o','n','=','L','O','G','O','.','I','C','O',0x0a,0x0d,
};

extern unsigned char logo_ico[];
extern unsigned int logo_ico_len;
// We are implementing a FAT12 filesystem!!!
//
// If(CountofClusters < 4085) {
//	   /* Volume is FAT12 */
// } else if(CountofClusters < 65525) {
//     /* Volume is FAT16 */
// } else {
//    /* Volume is FAT32 */
// }
// This is the one and only way that FAT type is determined.
// There is no such thing as a FAT12 volume that has more than 4084 clusters.
// There is no such thing as a FAT16 volume that has less than 4085 clusters
// or more than 65,524 clusters. There is no such thing as a FAT32 volume 
// that has less than 65,525 clusters. If you try to make a FAT volume 
// that violates this rule, Microsoft operating systems will not handle 
// them correctly because they will think the volume has a different 
// type of FAT than what you think it does.
int BlockDevInit(void)
{
	// USER_START_SECTOR refers to the flash sector, not the fat16 sector
	iapSectorToAddress(USER_START_SECTOR, &USER_FLASH_START, 0);
	USER_FLASH_SIZE = (0x0007CFFF - USER_FLASH_START) + 1;
	MSC_MemorySize = (RAMDISK_SIZE + USER_FLASH_SIZE);
	MSC_BlockCount = MSC_MemorySize / BLOCKSIZE;

	DBG("MSC Size=0x%04x, BlockCount=%d\n",MSC_MemorySize,MSC_BlockCount);

	// copy bootsect to ramdisk
	memcpy(&RamDisk[BOOTSECT_START], BootSect, sizeof(BootSect));

	// set total sectors (little endian, offset 0x13)
	RamDisk[BOOTSECT_START + 0x13] = (unsigned char)(MSC_BlockCount & 0xff);		// low byte
	RamDisk[BOOTSECT_START + 0x14] = (unsigned char)(MSC_BlockCount >> 8);			// high byte

	// There is one other important note about Sector 0 of a FAT volume.
	// If we consider the contents of the sector as a byte array,
	// it must be true that sector[510] equals 0x55, and sector[511] equals 0xAA. 
	RamDisk[BOOTSECT_START + 510] = 0x55;
	RamDisk[BOOTSECT_START + 511] = 0xAA;

	// initialize the File Allocation Table
	//const unsigned int numClusters = USER_FLASH_SIZE / (BLOCKSIZE * BLOCKS_PER_CLUSTER);
	//ASSERT(numClusters < 4085);
	memcpy(&RamDisk[FAT_START], FAT, sizeof(FAT));
	memcpy(&RamDisk[FAT_START + BLOCKSIZE], FAT, sizeof(FAT));	

	// initialize the root directory
	memcpy(&RamDisk[ROOTDIR_START], RootDir, sizeof(RootDir));

	// copy autorun.inf and logo.ico
	memcpy(&RamDisk[AUTORUN_START], autoruninf, sizeof(autoruninf));
	memcpy(&RamDisk[LOGO_START], logo_ico, logo_ico_len);
	
	return 0;
}

// should return the size in bytes of the block device through the output parameter
int BlockDevGetSize(U32 *pdwDriveSize)
{
	*pdwDriveSize = MSC_MemorySize;
	return 0;
}


// lfa - logical flash address; that is, should NOT contain the offset corresponding
// to the user flash start sector. The lfa starts at 0, and write_flash will add
// the correct offset to yeild the absolute flash address
static int write_flash(unsigned long lfa, const unsigned char *src, int numBytes) 
{
	int ret;
	unsigned long flashAddr = USER_FLASH_START + lfa;

	DBG("write_flash(): USER_FLASH_START = 0x%x, logical flash address = %d\n", USER_FLASH_START, lfa);

	// find the sector that contains the destination address
	int sector = iapAddressToSector(flashAddr, USER_START_SECTOR);
	DBG("write_flash(): preparing sector %d\n", sector);

	// if the address corresponds to the beginning of a sector, then erase it first
	unsigned long sectorStartAddress;
	iapSectorToAddress(sector, &sectorStartAddress, 0);
	if(flashAddr == sectorStartAddress) {
		DBG("write_flash(): erasing sector %d\n", sector);
		ret = iapPrepareSectors(sector, sector);
		if(ret != 0) {
			DBG("write_flash(): iapPrepareSectors() failed: %d\n", iapGetErrno());
		}
		ret = iapEraseSectors(sector, sector);
		if(ret != 0) {
			DBG("write_flash(): iapEraseSectors() failed: %d\n", iapGetErrno());
		}
	}

	ret = iapPrepareSectors(sector, sector);
	if(ret != 0) {
		DBG("write_flash(): iapPrepareSectors() failed! returned %d\n", ret);
		DBG("reason: %s\n", iapStrerror(ret));
		return -1;
	}
	unsigned char *tempBuf = (unsigned char *)0x40000000;
	memcpy(tempBuf, src, numBytes);
	ret = iapWriteSectors(flashAddr, tempBuf, numBytes);
	if(ret != 0) {
		DBG("write_flash(): iapWriteSectors(flashAddr = %d, src = 0x%x, numBytes = %d) failed: %d\n",
			flashAddr, (unsigned int)src, numBytes, iapGetErrno());
		return -1;
	}
	DBG("Write flash Done!\n");
	return 0;
}

int BlockDevWrite(U32 lba, const U8 * pbBuf)
{
	//static int writeEnabled = 0;

	// write to device
	DBG("BlockDevWrite(): lba = %d\n", lba);
	
	if (lba < 15) {
		DBG("Ramdisk write.\n");
		/*if(lba == 3 && pbBuf[0] == 'a' && pbBuf[1] == 'b') {
			// trap to supervisor on FIQ exit and turn on write enable.
			// need to write return address to FIQ_r11 and MODE_SVC to FIQ_SPSR
			DBG2("[BlockDevWrite(): programming start sequence received.]\n");
			//set_fiq_return(svc_enter);
			writeEnabled = 1;
		} else if(lba == 3 && pbBuf[0] == 'b' && pbBuf[1] == 'a') {
			// turn off write enable and jump to user code on FIQ return
			DBG2("[BlockDevWrite(): programming end sequence received. Will jump to user main on FIQ return.]\n");
			writeEnabled = 0;
			//set_fiq_return(start_user_main);
			//callUserProg();
		} else {
			// wants to write to the FAT or rootdir
			//DBG2("Writing to rootdir\n");
			memcpy(&RamDisk[BLOCKSIZE * lba], pbBuf, BLOCKSIZE);
		}*/
		memcpy(&RamDisk[BLOCKSIZE * lba], pbBuf, BLOCKSIZE);
	} else if(lba < MSC_BlockCount) {
		if(lba >=18) {
			unsigned int addr = BLOCKSIZE * (lba - 18);
			if(write_flash(addr, pbBuf, BLOCKSIZE) != 0) {
				DBG("BlockDevWrite(): write_flash failed!\n");
				return -1;
			}
		}
	} else {
		// out of bounds!
		return -1;
	}

	return 0;
}

// Want to read an entire block of data (BLOCKSIZE bytes) into pbBuf
// lba is a logical block address specifying the block number (0, 1, 2, ...)
// The real address can be found by: REAL ADDRESS = (logical block address) * BLOCKSIZE
int BlockDevRead(U32 lba, U8 * pbBuf)
{
	// read from device
	DBG("BlockDevRead(): lba = %d\n", lba);
	
	if(lba < 15) {
		// if they are requesting an lba in the boot sector, FAT, or root dir, read
		// from the ramdisk
		memcpy(pbBuf, &RamDisk[BLOCKSIZE * lba], BLOCKSIZE);
	} else if(lba < MSC_BlockCount) {
		// are they requesting an address in the flash space?
		// minus 4 because the boot sect, FAT, and rootdir take up 4 blocks
		memcpy(pbBuf, (char *)(USER_FLASH_START + BLOCKSIZE * (lba - 15)), BLOCKSIZE);
	} else {
		DBG("BlockDevRead(): lba is out of bounds! lba = %d\n", lba);
		// out of bounds!
		return -1;
	}

	return 0;
}

