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

	// init serial port (UART0)
	ConsoleInit(60000000 / (16 * BAUD_RATE));

	printf("Starting Test Routine from Windows...\n");
	
	printf("Serial and IO test program by Cackharot(cackharot@gmail.com)...\n");
	
	printf("Setting P0.14 as input pin...\n");
	printf("Setting P0.15 as output pin...\n");
	// set normal function mode (00)
	PINSEL0 &= ~(1 << 29 | 1 << 28); //28,29 bits are function select bits for P0.14 pin
	PINSEL0 &= ~(1 << 30 | 1 << 31);
	
	IODIR0=0xFFFF;
	IODIR0 &= ~(1 << 14); // make P0.14 as INPUT PIN
	
	IODIR0 |= (1<<15) | (1<<8);
	
	//turn on internal pull up
	IOSET0 = (1 << 14) | (1<<15) | (1<<8);
	
	while(1)
	{
		pinState = FIO0PIN;
	    printf("P0.14 State = 0x%x\n",getPinState(14));	 
		IOSET0 |= (1<<15) | (1<<8);
		delay_ms_t0(500);
	    IOCLR0 |= (1<<15) | (1<<8);
   	    delay_ms_t0(500);
	}
	
	return 0;
}

