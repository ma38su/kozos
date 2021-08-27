#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

int test10_1_main(int argc, char *argv[]);

static int start_threads(int argc, char *argv[]) {
  int stacksize = 0x100;
  kz_run(test10_1_main, "test10_1", 1, stacksize, 0, NULL);

  kz_chpri(15);
  INTR_ENABLE;

  while (1) {
    asm volatile ("sleep"); /* 省電力モード */
  }

  return 0;
}

int main(void) {
  INTR_DISABLE;

  puts("kozos boot succeed!\n");

  int priority = 0;
  int stacksize = 0x100;
  kz_start(start_threads, "idle", priority, stacksize, 0, NULL);

  return 0;
}

