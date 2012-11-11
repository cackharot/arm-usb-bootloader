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
#include "type.h"
#include "console.h"
#include "startup.h"
#include "LPC214x.h"
#include "sys.h"

#define BAUD_RATE	38400

int printf(const char *format, ...);
int getPinState(int pinNumber);

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
	int pinState = 0x0;
	// PLL and MAM
	Initialize();

	// init DBG
	ConsoleInit(60000000 / (16 * BAUD_RATE));

	printf("Starting Test Routine from Windows...\n");
	
	printf("Serial and IO test program by Cackharot(cackharot@gmail.com)...\n");
	
	printf("Setting P0.14 as input pin...\n");
	printf("Setting P0.15 as output pin...\n");
	// set normal function mode (00)
	PINSEL0 &= ~(1 << 29 | 1 << 28); //28,29 bits are function select bits for P0.14 pin
	PINSEL0 &= ~(1 << 30 | 1 << 31);
	
	FIO0DIR=0xFFFF;
	FIO0DIR &= ~(1 << 14); // make P0.14 as INPUT PIN
	
	FIO0DIR |= (1<<15);
	
	//turn on internal pull up
	FIO0SET = (1 << 14) | (1<<15);
	
	while(1)
	{
		pinState = FIO0PIN;
	    printf("P0.14 State = 0x%x\n",getPinState(14));	 
		FIO0SET |= (1<<15);
		delay_ms_t0(500);
	    FIO0CLR |= (1<<15);
   	    delay_ms_t0(500);
	}
	
	return 0;
}

