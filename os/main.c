#include "defines.h"
#include "kozos.h"
#include "interrupt.h"
#include "lib.h"

int test09_1_main(int argc, char *argv[]);
int test09_2_main(int argc, char *argv[]);
int test09_3_main(int argc, char *argv[]);
kz_thread_id_t test09_1_id;
kz_thread_id_t test09_2_id;
kz_thread_id_t test09_3_id;

static int start_threads(int argc, char *argv[]) {
  int stacksize = 0x100;
  test09_1_id = kz_run(test09_1_main, "test09_1", 1, stacksize, 0, NULL);
  test09_2_id = kz_run(test09_2_main, "test09_2", 2, stacksize, 0, NULL);
  test09_3_id = kz_run(test09_3_main, "test09_3", 3, stacksize, 0, NULL);

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

