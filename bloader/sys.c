#include "LPC214x.h"
#include "sys.h"
 
// delay for the specified number of ms using timer0
void delay_ms_t0(int ms)
{
	// use the timer to set an interrupt and wait until it fires
	T0TCR = 0x02;				// disable timer, put in reset mode.
	T0CTCR = 0x00;				// select timer mode
	T0PR = 60000 - 1; 				
	T0MR0 = ms;
	T0MCR = (1 << 2);			// when timer0 matches the match0 register, the timer will be disabled
	T0TCR = 0x01;				// take the timer out of reset mode and enable for counting.
	
	while((T0TCR & 0x01) == 1);	// wait for the timer to be disabled
}

// delay for the specified number of microseconds using timer0
void delay_us_t0(int us)
{
	// use the timer to set an interrupt and wait until it fires
	T0TCR = 0x02;				// disable timer, put in reset mode.
	T0CTCR = 0x00;				// select timer mode
	T0PR = 14; 					// prescale register to 15 for 15MHz PCLK - william
	T0MR0 = us;
	T0MCR = (1 << 2);			// when timer0 matches the match0 register, the timer will be disabled
	T0TCR = 0x01;				// take the timer out of reset mode and enable for counting.
	
	while((T0TCR & 0x01) == 1);	// wait for the timer to be disabled
}

// delay for the specified number of ms using timer1
void delay_ms_t1(int ms)
{
	// use the timer to set an interrupt and wait until it fires
	T1TCR = 0x02;				// disable timer, put in reset mode.
	T1CTCR = 0x00;				// select timer mode
	T1PR = 15000-1; 				// Changed from 60,000 to 15,000 for 1 KHz clock - william
	T1MR0 = ms;
	T1MCR = (1 << 2);			// when timer0 matches the match0 register, the timer will be disabled
	T1TCR = 0x01;				// take the timer out of reset mode and enable for counting.
	
	while((T1TCR & 0x01) == 1);	// wait for the timer to be disabled
}

// delay for the specified number of microseconds using timer1
void delay_us_t1(int us)
{
	// use the timer to set an interrupt and wait until it fires
	T1TCR = 0x02;				// disable timer, put in reset mode.
	T1CTCR = 0x00;				// select timer mode
	T1PR = 14; 					// Changed from 60 to 15, prescale. - william
	T1MR0 = us;
	T1MCR = (1 << 2);			// when timer0 matches the match0 register, the timer will be disabled
	T1TCR = 0x01;				// take the timer out of reset mode and enable for counting.
	
	while((T1TCR & 0x01) == 1);	// wait for the timer to be disabled
}

unsigned enableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = asm_get_cpsr();
  asm_set_cpsr(_cpsr & ~IRQ_MASK);
  return _cpsr;
}

unsigned disableIRQ(void)
{
  unsigned _cpsr;

  _cpsr = asm_get_cpsr();
  asm_set_cpsr(_cpsr | IRQ_MASK);
  return _cpsr;
}

unsigned restoreIRQ(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = asm_get_cpsr();
  asm_set_cpsr((_cpsr & ~IRQ_MASK) | (oldCPSR & IRQ_MASK));
  return _cpsr;
}

unsigned enableFIQ(void)
{
  unsigned _cpsr;

  _cpsr = asm_get_cpsr();
  asm_set_cpsr(_cpsr & ~FIQ_MASK);
  return _cpsr;
}

unsigned disableFIQ(void)
{
  unsigned _cpsr;

  _cpsr = asm_get_cpsr();
  asm_set_cpsr(_cpsr | FIQ_MASK);
  return _cpsr;
}

unsigned restoreFIQ(unsigned oldCPSR)
{
  unsigned _cpsr;

  _cpsr = asm_get_cpsr();
  asm_set_cpsr((_cpsr & ~FIQ_MASK) | (oldCPSR & FIQ_MASK));
  return _cpsr;
}
