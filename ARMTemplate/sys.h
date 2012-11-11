#ifndef _SYS_H
#define _SYS_H

#include "LPC214x.h"

#define DBG2 DBG

#define LE_WORD(x)		((x)&0xFF),((x)>>8)

// delay for the specified number of milliseconds using timer0
void delay_ms_t0(int ms);

// delay for the specified number of microseconds using timer0
void delay_us_t0(int us);

// delay for the specified number of milliseconds using timer1
void delay_ms_t1(int ms);

// delay for the specified number of microseconds using timer1
void delay_us_t1(int us);

#define IRQ_MASK 0x80
#define FIQ_MASK 0x40

static inline unsigned asm_get_cpsr(void)
{
	unsigned long retval;
	asm volatile (" mrs  %0, cpsr" : "=r" (retval) : /* no inputs */  );
	return retval;
}

static inline void asm_set_cpsr(unsigned val)
{
	asm volatile (" msr  cpsr, %0" : /* no outputs */ : "r" (val)  );
}

unsigned enableIRQ(void);
unsigned disableIRQ(void);
unsigned restoreIRQ(unsigned oldCPSR);

unsigned enableFIQ(void);
unsigned disableFIQ(void);
unsigned restoreFIQ(unsigned oldCPSR);

#endif // _SYS_H
