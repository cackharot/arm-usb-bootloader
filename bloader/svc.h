#ifndef _SVC_H
#define _SVC_H

#include <sys/stat.h>

// Give a function which control should be passed to on
// return from the FIQ routine. The function will be
// executed in SVC mode with all interrupts disabled
void set_fiq_return(void (*func)());

/*
This is called after an abort or undefined exception, or after
the user has sent the begin-program command code over USB.
We should just hang out and let the USB interrupt routine do
do its thing.
*/
void svc_idle() __attribute__ ((noreturn));


/*
 * void svc_enter();
 *
 * Switches to SVC mode with interrupts disabled,
 * issues a soft reset, resets the SVC stack pointer,
 * then enables FIQ interrupts, then branches to svc_idle();
 *
 */
void svc_enter() __attribute__ ((noreturn));

/**
 * Initialize user stack, switch to MODE_USR, enable interrupts,
 * and jump to user code. This function does not return.
 */
void start_user_main();

// syscalls
int svc_write (int file, const char *ptr, int len);
int svc_read (int file, char *ptr, int len);
int svc_close(int file);
_off_t svc_lseek(int file, _off_t offset, int dir);
int svc_fstat(int file, struct stat *st);
int svc_isatty (int file);
int svc_open(const char *name, int flags, int mode);

#endif /* _SVC_H */
