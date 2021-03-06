#include "defines.h"
#include "serial.h"

#define SERIAL_SCI_NUM 3

#define H8_3069F_SCI0 ((volatile struct h8_3069f_sci *)0xffffb0)
#define H8_3069F_SCI1 ((volatile struct h8_3069f_sci *)0xffffb8)
#define H8_3069F_SCI2 ((volatile struct h8_3069f_sci *)0xffffc0)

struct h8_3069f_sci {
  volatile uint8_t smr;
  volatile uint8_t brr;
  volatile uint8_t scr;
  volatile uint8_t tdr;
  volatile uint8_t ssr;
  volatile uint8_t rdr;
  volatile uint8_t scmr;
};

#define H8_3069F_SCI_SMR_CKS_PER1  (0<<0)
#define H8_3069F_SCI_SMR_CKS_PER4  (1<<0)
#define H8_3069F_SCI_SMR_CKS_PER16 (2<<0)
#define H8_3069F_SCI_SMR_CKS_PER64 (3<<0)
#define H8_3069F_SCI_SMR_MP        (1<<2)
#define H8_3069F_SCI_SMR_STOP      (1<<3)
#define H8_3069F_SCI_SMR_OE        (1<<4)
#define H8_3069F_SCI_SMR_PE        (1<<5)
#define H8_3069F_SCI_SMR_CHR       (1<<6)
#define H8_3069F_SCI_SMR_CA        (1<<7)

#define H8_3069F_SCI_SCR_CKE0   (1<<0)
#define H8_3069F_SCI_SCR_CKE1   (1<<1)
#define H8_3069F_SCI_SCR_TEIE   (1<<2)
#define H8_3069F_SCI_SCR_MPIE   (1<<3)
#define H8_3069F_SCI_SCR_RE     (1<<4) /* 受信成功 */
#define H8_3069F_SCI_SCR_TE     (1<<5) /* 送信成功 */
#define H8_3069F_SCI_SCR_RIE    (1<<6) /* 受信割込み成功 */
#define H8_3069F_SCI_SCR_TIE    (1<<7) /* 受信割込み失敗 */

#define H8_3069F_SCI_SSR_MPBT   (1<<0)
#define H8_3069F_SCI_SSR_MPB    (1<<1)
#define H8_3069F_SCI_SSR_TEND   (1<<2)
#define H8_3069F_SCI_SSR_PER    (1<<3)
#define H8_3069F_SCI_SSR_FERERS (1<<4)
#define H8_3069F_SCI_SSR_ORER   (1<<5)
#define H8_3069F_SCI_SSR_RDRF   (1<<6) /* 受信完了 */
#define H8_3069F_SCI_SSR_TDRE   (1<<7) /* 送信完了 */

static struct {
  volatile struct h8_3069f_sci *sci;
} regs[SERIAL_SCI_NUM] = {
  { H8_3069F_SCI0 },
  { H8_3069F_SCI1 },
  { H8_3069F_SCI2 },
};

int serial_init(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;

  sci->scr = 0;
  sci->smr = 0;
  sci->brr = 64;
  sci->scr = H8_3069F_SCI_SCR_RE | H8_3069F_SCI_SCR_TE;
  sci->ssr = 0;

  return 0;
}

int serial_is_send_enable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  return (sci->ssr & H8_3069F_SCI_SSR_TDRE);
}

int serial_send_byte(int index, unsigned char c) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;

  while (!serial_is_send_enable(index));

  sci->tdr = c;
  sci->ssr &= ~H8_3069F_SCI_SSR_TDRE;

  return 0;
}

int serial_is_recv_enable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  return (sci->ssr & H8_3069F_SCI_SSR_RDRF);
}

/* 1文字受信 */
unsigned char serial_recv_byte(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  unsigned char c;

  /* 受信文字が来るまで待つ */
  while (!serial_is_recv_enable(index))
    ;
  c = sci->rdr;
  sci->ssr &= ~H8_3069F_SCI_SSR_RDRF; /* 受信完了 */

  return c;
}

int serial_intr_is_send_enable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  return (sci->scr & H8_3069F_SCI_SCR_TIE) ? 1 : 0;
}

void serial_intr_send_enable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr |= H8_3069F_SCI_SCR_TIE;
}

void serial_intr_send_disable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr &= ~H8_3069F_SCI_SCR_TIE;
}

int serial_intr_is_recv_enable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  return (sci->scr & H8_3069F_SCI_SCR_RIE) ? 1 : 0;
}

void serial_intr_recv_enable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr |= H8_3069F_SCI_SCR_RIE;
}

void serial_intr_recv_disable(int index) {
  volatile struct h8_3069f_sci *sci = regs[index].sci;
  sci->scr &= ~H8_3069F_SCI_SCR_RIE;
}

