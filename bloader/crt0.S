/* ***************************************************************************************************************

	crt.s						STARTUP  ASSEMBLY  CODE 
								-----------------------


	Module includes the interrupt vectors and start-up code.

  *************************************************************************************************************** */

/* Standard definitions of Mode bits and Interrupt (I & F) flags in PSRs (program status registers) */
.set  MODE_USR, 0x10            		/* Normal User Mode 										*/
.set  MODE_FIQ, 0x11            		/* FIQ Processing Fast Interrupts Mode 						*/
.set  MODE_IRQ, 0x12            		/* IRQ Processing Standard Interrupts Mode 					*/
.set  MODE_SVC, 0x13            		/* Supervisor Processing Software Interrupts Mode 			*/
.set  MODE_ABT, 0x17            		/* Abort Processing memory Faults Mode 						*/
.set  MODE_UND, 0x1B            		/* Undefined Processing Undefined Instructions Mode 		*/
.set  MODE_SYS, 0x1F            		/* System Running Priviledged Operating System Tasks  Mode	*/

.set  I_BIT, 0x80               		/* when I bit is set, IRQ is disabled (program status registers) */
.set  F_BIT, 0x40               		/* when F bit is set, FIQ is disabled (program status registers) */
.set  NO_INT, (I_BIT | F_BIT)			/*mask to disable IRQ and FIQ */

.set  PCONP,			0xE01FC0C4
.set  VICVectAddr,		0xFFFFF030

.text
.arm

.global	Reset_Handler
.global _startup
.func   _startup

_startup:

# Exception Vectors

_vectors:       ldr     PC, Reset_Addr         
                ldr     PC, Undef_Addr
                ldr     PC, SWI_Addr
                ldr     PC, PAbt_Addr
                ldr     PC, DAbt_Addr
                nop							/* Reserved Vector (holds Philips ISP checksum) */
                ldr     PC, IRQ_Addr		/* see page 71 of "Insiders Guide to the Philips ARM7-Based Microcontrollers" by Trevor Martin  */
                ldr     PC, FIQ_Addr

Reset_Addr:     .word   Reset_Handler
Undef_Addr:     .word   UNDEF_Routine
SWI_Addr:       .word   ARM_swi
PAbt_Addr:      .word   UNDEF_Routine
DAbt_Addr:      .word   UNDEF_Routine
IRQ_Addr:       .word   ARM_irq
FIQ_Addr:       .word   ARM_fiq	
	
				.word   0					/* rounds the vectors and ISR addresses to 64 bytes total  */


# Reset Handler

Reset_Handler:  

	/* Turn on USB to enable USB DMA RAM area */
	ldr		r1, =PCONP				// load address of PCONP register
	ldr		r2, [r1]				// load value of PCONP
	mov		r3, #1					// load 1
	orr		r2, r3, lsl #31			// set bit 31
	str		r2, [r1]				// store back into PCONP

	/* Setup a stack for each mode - note that this only sets up a usable stack
	for User mode.   Also each mode is setup with interrupts initially disabled. */
    			 
	// Set up stack for every mode but user/system mode.
	// The bootloader will run in SVC mode
	msr   CPSR_c, #MODE_UND|I_BIT|F_BIT 	/* Undefined Instruction Mode  */
	msr   CPSR_c, #MODE_ABT|I_BIT|F_BIT 	/* Abort Mode */
	msr   CPSR_c, #MODE_FIQ|I_BIT|F_BIT 	/* FIQ Mode */
	msr   CPSR_c, #MODE_IRQ|I_BIT|F_BIT 	/* IRQ Mode */
	msr   CPSR_c, #MODE_SVC|I_BIT|F_BIT 	/* Supervisor Mode */
	//ldr   sp, =_svc_stack_end
	ldr   sp, =_user_stack_end

	/* copy .data section (Copy from ROM to RAM) */
	ldr     R1, =_flash_data_start
	ldr     R2, =_data
	ldr     R3, =_edata
1:	
	cmp     R2, R3
	ldrlo   R0, [R1], #4
	strlo   R0, [R2], #4
	blo     1b

	/* Clear .bss section (Zero init)  */
	mov     R0, #0
	ldr     R1, =_bss_start
	ldr     R2, =_bss_end
2:  
	cmp     R1, R2
	strlo   R0, [R1], #4
	blo     2b

	/* Enter the C code  */
	b       main

.endfunc

// table of function pointers for SWI routines
.align 4
SWItbl:

/*

We are in SVC mode with FIQ on, IRQ off.

Need to save r12 (which we are using as a scratch register),
and lr, which will get overwritten when we branch and link
to the SWI. I suppose we don't REALLY need to save r12,
but the ABI requires that we keep the stack pointer 
double word aligned, so we should push 2 regs to the stack.

*/
dbgstr: .asciz "swi number: %d\n"

.func ARM_swi
ARM_swi:
	push	{r12, lr}				// save r12 and lr
	ldr		r12, [lr, #-4]			// get the instruction which caused the SWI
	bic		r12,r12,#0xff000000		// mask off SWI bits
	
	cmp		r12, #8					// bounds check
	bhs		.L8

	ldr  	lr,=SWItbl				// load base of jump array
	add		r12,lr,r12,lsl #2		// compute jump address
	mov		lr,pc					// save return address
	ldr		pc, [r12]				// jump to SWI routine
	
.L8:
	pop		{r12, lr}				// restore r12 and lr from the SVC stack
	movs	pc,lr					// return from interrupt restoring CPSR from SPSR
.endfunc

/*

ARM_irq can come only from user, system, or SVC mode. The state
always gets saved on the SVC stack, and the handling of the IRQ is
dispatched to whatever mode we came from.

It disables IRQ interrupts, but does NOT disable FIQ interrupts.
This is so that if the user code enters an infinite loop or crashes,
the USB interface will still be responsive.

We need to save r0-r3, r12-14 from the mode that was executing before
the interrupt was triggered.

We do not trust the user stack. We should save and restore the user registers
faithfully, but we should not do any memory accesses based on the stack pointer
or link register from user mode. We should only access memory using the
supervisor stack.

r0 gets saved in r13_IRQ and r1 gets saved r14_IRQ.

*/
.func ARM_irq
ARM_irq:
	sub		lr,lr,#4					// compute return address
	ldr		r13, =_svc_stack_end		// load address of register save area
	stm		r13, {r0-r3,r12}				// still need to save lr of source mode
	
	mrs		r0, SPSR					// get source mode
	orr		r0, r0, #NO_INT				// turn off interrupts in source mode
	msr		CPSR_c,r0					// switch to source mode with all interrupts off

	ldr		r12, =_svc_stack_end		// we have 24 bytes at the top of ram to use for storage
	str		lr, [r12, #20]				// save lr to register save area. Register save area now has r0-r3, r12, lr

	and		r0, r0, #~F_BIT				// turn on FIQ interrupts
	msr		CPSR_c,r0

	/* Enter user IRQ C Code by jumping to value of VICVectAddr */
	LDR		r12,=VICVectAddr			// VICVectAddr: contains pointer to C function to execute */
	MOV		lr,pc						// copy the return address to link register */
	ldr		pc, [r12]
	/* Exit user C code */

	ldr		r12,=VICVectAddr			// write 0 to VICVectAddr
	mov		lr, #0
	str		lr, [r12]
	
	/*
	mrs		r12, CPSR					// get current mode
	orr		r12, r12, #NO_INT			// turn off all interrupts
	msr		CPSR_c,r12
	*/

	ldr		r12, =_svc_stack_end		// restore lr
	ldr		lr, [r12, #20]

	msr		CPSR_c,#(MODE_IRQ | NO_INT)	// change to IRQ mode
	ldm		r13, {r0-r3, r12}			// restore value of r0-r3, r12
	movs	pc,lr						// return restoring CPSR from SPSR
.endfunc

// When we enter FIQ, we could be coming from any other mode (except FIQ)
// We need to save all state and switch to SVC mode before
// handling the interrupt. Need to save r0-r3, r12, lr
.func ARM_fiq
ARM_fiq: 
	sub		lr,lr,#4						// save return address in r14_fiq (where to resume execution after interrupt)
	mov		r8, r0							// save clobbered regs 
	mov		r9, r1							// regs r4-r14 excluding r12 are callee-saved,
	mov		r10, r2							// so we don't need to save them
	mov		r11, r3
								
	MSR     cpsr_c,#(MODE_SVC | NO_INT)		// switch to SVC mode and handle interrupt
	push	{r12,lr}						// push r12,lr to the SVC stack
	bl		USBHwISR						// handle the interrupt
	pop		{r12,lr}						// restore r12,lr from the SVC stack

	// need to restore all registers. restore r12-24 from FIQ mode to previous mode
	msr     cpsr_c,#(MODE_FIQ | NO_INT)		// switch to FIQ mode
	mov		r0, r8
	mov		r1, r9
	mov		r2, r10
	mov		r3, r11

	MOVS    pc, lr							// return restoring CPSR from SPSR
	
.endfunc


// handles Data Abort, Prefetch Abort, and Undefined exceptions.
// Should check to see which mode we came from. If we came from
// user mode, system mode, or IRQ mode, we need to print out a message
// then switch to SVC mode and call svc_idle(), which will just hang
// out until the user programs new firmware.
// If we came from FIQ mode, SVC mode, Undefined, or Abort mode that means we've got a bug
// in the supervisor. Display blue screen of death.
.func UNDEF_Routine
UNDEF_Routine:
	// if we got here, then we're bad.
	b .
.endfunc

.end
