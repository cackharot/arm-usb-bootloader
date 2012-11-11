/*
	LPCUSB, an USB device driver for LPC microcontrollers	
	Copyright (C) 2006 Bertrik Sikken (bertrik@sikken.nl)

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright
	   notice, this list of conditions and the following disclaimer.
	2. Redistributions in binary form must reproduce the above copyright
	   notice, this list of conditions and the following disclaimer in the
	   documentation and/or other materials provided with the distribution.
	3. The name of the author may not be used to endorse or promote products
	   derived from this software without specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
	IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
	OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
	IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, 
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
	NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
	DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
	THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
	THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "type.h"
#include "usbdebug.h"

#include "console.h"
#include "usbapi.h"
#include "startup.h"

#include "LPC214x.h"

/*
#undef DBG
#undef DBG2
#define DBG printf
#define DBG2 DBG
*/
#include "sys.h"
#include "iap.h"
#include "svc.h"
#include "msc_bot.h"
#include "blockdev.h"

#define EP2IDX(bEP)	((((bEP)&0xF)<<1)|(((bEP)&0x80)>>7))

#define BAUD_RATE	38400

#define MAX_PACKET_SIZE	64

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

typedef void    (*voidFP)( void );
void callUserProg(void);
void soft_reset();
void USB_stop(void);
void dump( U32 addr, int len );

int getPinState(int pinNumber);
int checkUserCodePresent();

static int isUSBEnabled = 0;
/****************************************************************************
						Begin USB Descriptors
****************************************************************************/

static U8 abClassReqData[4];

static const U8 abDescriptors[] = {
// device descriptor	
	0x12,
	DESC_DEVICE,			
	LE_WORD(0x0200),		// bcdUSB
	0x00,					// bDeviceClass
	0x00,					// bDeviceSubClass
	0x00,					// bDeviceProtocol
	MAX_PACKET_SIZE0,		// bMaxPacketSize
	LE_WORD(0xFAAB),		// idVendor
	LE_WORD(0x0001),		// idProduct
	LE_WORD(0x0100),		// bcdDevice
	0x01,					// iManufacturer
	0x02,					// iProduct
	0x03,					// iSerialNumber
	0x01,					// bNumConfigurations

// configuration descriptor
	0x09,
	DESC_CONFIGURATION,
	LE_WORD(32),			// wTotalLength
	0x01,					// bNumInterfaces
	0x01,					// bConfigurationValue
	0x00,					// iConfiguration
	0x80,					// bmAttributes
	0x32,					// bMaxPower

// interface
	0x09,
	DESC_INTERFACE,
	0x00,					// bInterfaceNumber
	0x00,					// bAlternateSetting
	0x02,					// bNumEndPoints
	0x08,					// bInterfaceClass = mass storage
	0x06,					// bInterfaceSubClass = transparent SCSI
	0x50,					// bInterfaceProtocol = BOT
	0x00,					// iInterface
// EP
	0x07,
	DESC_ENDPOINT,
	MSC_BULK_IN_EP,			// bEndpointAddress
	0x02,					// bmAttributes = bulk
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0x00,					// bInterval
// EP
	0x07,
	DESC_ENDPOINT,
	MSC_BULK_OUT_EP,		// bEndpointAddress
	0x02,					// bmAttributes = bulk
	LE_WORD(MAX_PACKET_SIZE),// wMaxPacketSize
	0x00,					// bInterval

// string descriptors
	0x04,
	DESC_STRING,
	LE_WORD(0x0409),

	// manufacturer
	24,
	DESC_STRING,
	'I', 0, 'N', 0, 'T', 0, 'E', 0, 'R', 0, 'L', 0, 'O', 0, 'G', 0, 'I', 0, 'X', 0, ' ', 0,

	// product
	40,
	DESC_STRING,
	'L', 0, 'P', 0, 'C', 0, ' ', 0, '2', 0, '1', 0, '4', 0, '8', 0, ' ', 0,
	'B', 0, 'O', 0, 'O', 0, 'T', 0, 'L', 0, 'O', 0, 'A', 0, 'D', 0, 'E', 0, 'R', 0,

	// Serial number
	36,
	DESC_STRING,
	'L', 0, 'P', 0, 'C', 0, '2', 0, '1', 0, '4', 0, '8', 0,
	'B', 0, 'O', 0, 'O', 0, 'T', 0, 'L', 0, 'O', 0, 'A', 0, 'D', 0, 'E', 0, 'R', 0,
	
	// First interface description
	38,
	DESC_STRING,
	'L', 0, 'P', 0, 'C', 0, '2', 0, '1', 0, '4', 0, '8', 0,
	' ', 0, 'F', 0, 'l', 0, 'a', 0, 's', 0, 'h', 0, ' ', 0, 'D', 0, 'i', 0, 's', 0, 'k', 0,

	// terminating zero
	0
};

/*************************************************************************
	HandleClassRequest
	==================
		Handle mass storage class request
	
**************************************************************************/
static BOOL HandleClassRequest(TSetupPacket *pSetup, int *piLen, U8 **ppbData)
{
	if (pSetup->wIndex != 0) {
		DBG("Invalid idx %X\n", pSetup->wIndex);
		return FALSE;
	}
	if (pSetup->wValue != 0) {
		DBG("Invalid val %X\n", pSetup->wValue);
		return FALSE;
	}

	switch (pSetup->bRequest) {

	// get max LUN
	case 0xFE:
		*ppbData[0] = 0;		// No LUNs
		*piLen = 1;
		break;

	// MSC reset
	case 0xFF:
		if (pSetup->wLength > 0) {
			return FALSE;
		}
		MSCBotReset();
		break;
		
	default:
		DBG("Unhandled class\n");
		return FALSE;
	}
	return TRUE;
}

#define INTR_IN_EP		0x81
#define REPORT_SIZE		4
static U8	abReport[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
static int	_iFrame = 0;

static void HandleFrame(U16 wFrame)
{
	static int iCount;
	_iFrame++;
	if ((_iFrame > 1000)) {
		// send report (dummy data)
		abReport[0] = (iCount >> 8) & 0xFF;
		abReport[1] = (iCount) & 0xFF;
		iCount++;
		USBHwEPWrite(INTR_IN_EP, abReport, REPORT_SIZE);
		_iFrame = 0;
	}
	(void)wFrame;
}

void usb_bootloader(void)
{
	// initialise the SD card
	BlockDevInit();

	DBG("Initialising USB stack\n");

	// initialise stack
	USBInit();
	
	// enable bulk-in interrupts on NAKs
	// these are required to get the BOT protocol going again after a STALL
	USBHwNakIntEnable(INACK_BI);

	// register descriptors
	USBRegisterDescriptors(abDescriptors);

	// register class request handler
	USBRegisterRequestHandler(REQTYPE_TYPE_CLASS, HandleClassRequest, abClassReqData);
	
	// register endpoint handlers for MSC interface
	USBHwRegisterEPIntHandler(MSC_BULK_IN_EP, MSCBotBulkIn);
	USBHwRegisterEPIntHandler(MSC_BULK_OUT_EP, MSCBotBulkOut);
	
	// register frame handler
	USBHwRegisterFrameHandler(HandleFrame);
	
	VICIntSelect = 1 << 22;			// classify USB as FIQ interrupt
	
	DBG("Starting USB communication\n");

	//enableIRQ();
	enableFIQ();
	
	// connect to bus
	USBHwConnect(TRUE);

	VICIntEnable |= (1<<22);
	
	isUSBEnabled = 1;
}

/**/
int getPinState(int pinNumber)
{
  // Read the current state of all pins in GPIO block 0
  int pinBlockState = FIO0PIN;

  // Read the value of 'pinNumber'
  int pinState = (pinBlockState & (1 << pinNumber)) ? 1 : 0;

  // Return the value of pinState
  return pinState;
}

/*************************************************************************
	main
	====
**************************************************************************/
int main(void)
{
	// PLL and MAM
	Initialize();

	// init DBG
	ConsoleInit(60000000 / (16 * BAUD_RATE));
	
	// set normal function mode (00)
	PINSEL0 &= ~(1 << 29 | 1 << 28); //28,29 bits are function select bits for P0.14 pin
	
	FIO0DIR &= ~(1 << 15); // make P0.15 as INPUT PIN
	
	// if the BSL pin is pulled high (P0.14)
	// and user code is present 
	// then execute user code.
	if(getPinState(15) == 1 && checkUserCodePresent() == 1) {
		DBG2("Calling user main()\n");
		callUserProg();
	} 
		
	DBG2("Entering bootloader...\n");
	usb_bootloader(); // setup usb bootloader and enter bloader mode
		
	while(1) {
		DBG("idle.\n");
		delay_ms_t0(1000);
	}
	
	return 0;
}

/**/
int checkUserCodePresent() {
	return iapBlankCheckSectors(USER_START_SECTOR,USER_START_SECTOR);
}

/**/
void callUserProg(void) {
	voidFP prog = (voidFP)USER_CODE_ADDR;
	DBG2("Calling user main()\n");		
	dump(USER_CODE_ADDR,512);
	if(isUSBEnabled)
		USB_stop();
	delay_ms_t0(1000);
	disableIRQ( );
	memcpy((void *)0x40000000, (void *)prog, 64);
	MEMMAP = (MEMMAP & ~3) | 2; // Set RAM vectors
	prog();		
}

void USB_stop(void)
{
 USBHwConnect(FALSE);
 USBDevIntEn=0;
}

void dump( U32 addr, int len )
{
    int  i, j;
    const U8 *p = (const U8 *)addr;

    for (i = 0; i < len; i += 16) {
        DBG2( "%04x : ", addr + i );
        for (j = 0; j < 16; ++j) {
            if (i + j < len) {
                DBG2( "%02x ", p[i+j] );
            } else {
                DBG2( "   " );
            }
        }
        DBG2(" ");                        
        DBG2("-");
        DBG2(" ");        
        for (j = 0; (j < 16) && (i + j < len); ++j) {
            if (isprint(p[i+j])) {
                DBG2("%c",p[i+j]);
            } else {
                DBG2(".");
            }
        }    
        DBG2("\n");
    }
}
