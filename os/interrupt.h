#ifndef _INTERRUPT_H_INCLUDED_
#define _INTERRUPT_H_INCLUDED_

extern char softvec;
#define SOFTVEC_ADDR (&softvec)

typedef short softvec_type_t;

typedef void (*softvec_handler_t)(softvec_type_t type, unsigned long sp);

#define SOFTVECS ((softvec_handler_t *)SOFTVEC_ADDR)

#define INTR_ENABLE  asm volatile ("andc.b #0x3f,ccr") /* 割込み有効 */
#define INTR_DISABLE asm volatile ("orc.b #0xc0,ccr")  /* 割り込み禁止 */


int softvec_init(void);

int softvec_setintr(softvec_type_t type, softvec_handler_t handler);

void interrupt(softvec_type_t type, unsigned long sp);

#endif
