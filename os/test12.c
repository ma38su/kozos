#include "defines.h"
#include "kozos.h"
#include "lib.h"

int test12_1_main(int argc, char *argv[]) {
  char *p;
  int size;

  puts("test11_1 started.\n");

  puts("test11_1 recv in.\n");
  kz_recv(MSGBOX_ID_MSGBOX1, &size, &p);
  puts("test11_1 recv out.\n");
  puts(p);

  puts("test11_1 recv in.\n");
  kz_recv(MSGBOX_ID_MSGBOX1, &size, &p);
  puts("test11_1 recv out.\n");
  puts(p);
  kz_kmfree(p);

  puts("test11_1 send in.\n");
  kz_send(MSGBOX_ID_MSGBOX2, 16, "static memory1\n");
  puts("test11_1 send out.\n");

  p = kz_kmalloc(19);
  strcpy(p, "allocated memory1\n");
  puts("test11_1 send in.\n");
  kz_send(MSGBOX_ID_MSGBOX2, 19, p);
  puts("test11_1 send out.\n");

  puts("test11_1 exit.\n");

  return 0;
}


