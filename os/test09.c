#include "defines.h"
#include "kozos.h"
#include "lib.h"

extern kz_thread_id_t test09_1_id;
extern kz_thread_id_t test09_2_id;

int test09_1_main(int argc, char *argv[]) {
  puts("test09_1 started.\n");

  puts("test09_1 sleep in.\n");
  kz_sleep();
  puts("test09_1 sleep out.\n");

  puts("test09_1 chpri(3) in.\n");
  kz_chpri(3);
  puts("test09_1 chpri(3) out.\n");

  puts("test09_1 wait in.\n");
  kz_wait();
  puts("test09_1 wait out.\n");

  puts("test09_1 trap in.\n");
  asm volatile("trapa #1");
  puts("test09_1 trap out.\n");

  puts("test09_1 exit.\n");

  return 0;
}

int test09_2_main(int argc, char *argv[]) {
  puts("test09_2 started.\n");

  puts("test09_2 sleep in.\n");
  kz_sleep();
  puts("test09_2 sleep out.\n");

  puts("test09_2 chpri(3) in.\n");
  kz_chpri(3);
  puts("test09_2 chpri(3) out.\n");

  puts("test09_2 wait in.\n");
  kz_wait();
  puts("test09_2 wait out.\n");

  puts("test09_2 trap in.\n");
  asm volatile("trapa #1");
  puts("test09_2 trap out.\n");

  puts("test09_2 exit.\n");

  return 0;
}

int test09_3_main(int argc, char *argv[]) {
  puts("test09_3 started.\n");

  puts("test09_3 wakeup in (test09_1).\n");
  kz_wakeup(test09_1_id);
  puts("test09_3 wakeup out (test09_1).\n");

  puts("test09_3 wakeup in (test09_2).\n");
  kz_wakeup(test09_2_id);
  puts("test09_3 wakeup out (test09_2).\n");

  puts("test09_3 wait in.\n");
  kz_wait();
  puts("test09_3 wait out.\n");

  puts("test09_3 exit in.\n");
  kz_exit();
  puts("test09_3 exit out.\n");

  return 0;
}

