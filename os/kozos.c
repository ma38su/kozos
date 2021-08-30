#include "kozos.h"
#include "intr.h"
#include "interrupt.h"
#include "syscall.h"
#include "memory.h"
#include "lib.h"

#define THREAD_NUM 6
#define PRIORITY_NUM 16
#define THREAD_NAME_SIZE 15

typedef struct _kz_context {
  uint32_t sp;
} kz_context;

// TCB (task control block)
typedef struct _kz_thread {
  struct _kz_thread *next;
  char name[THREAD_NAME_SIZE + 1];
  int priority;
  char *stack;
  uint32_t flags;
#define KZ_THREAD_FLAG_READY (1 << 0)

  struct {
    kz_func_t func;
    int argc;
    char **argv;
  } init;

  struct {
    kz_syscall_type_t type;
    kz_syscall_param_t *param;
  } syscall;

  kz_context context;
} kz_thread;

typedef struct _kz_msgbuf {
  struct _kz_msgbuf *next;
  kz_thread *sender;
  struct {
    int size;
    char *p;
  } param;
} kz_msgbuf;

typedef struct _kz_msgbox {
  kz_thread *receiver;
  kz_msgbuf *head;
  kz_msgbuf *tail;

  long dummpy[1];
} kz_msgbox;

static struct {
  kz_thread *head;
  kz_thread *tail;
} readyque[PRIORITY_NUM];

static kz_thread *current;
static kz_thread threads[THREAD_NUM];
static kz_handler_t handlers[SOFTVEC_TYPE_NUM];
static kz_syscall_func_t syscall_funcs[KZ_SYSCALL_TYPE_NUM];
static kz_msgbox msgboxes[MSGBOX_ID_NUM];

void dispatch(kz_context *context);

static int getcurrent(void) {
  if (current == NULL) {
    return -1;
  }
  if (!(current->flags & KZ_THREAD_FLAG_READY)) {
    return 1;
  }

  readyque[current->priority].head = current->next;
  if (readyque[current->priority].head == NULL) {
    readyque[current->priority].tail = NULL;
  }
  current->flags &= ~KZ_THREAD_FLAG_READY;
  current->next = NULL;

  return 0;
}

static int putcurrent(void) {
  if (current == NULL) {
    return -1;
  }
  if (current->flags & KZ_THREAD_FLAG_READY) {
    return 1;
  }

  if (readyque[current->priority].tail) {
    readyque[current->priority].tail->next = current;
  } else {
    readyque[current->priority].head = current;
  }
  readyque[current->priority].tail = current;
  current->flags |= KZ_THREAD_FLAG_READY;

  return 0;
}

static void thread_end(void) {
  kz_exit();
}

static void thread_init(kz_thread *thp) {
  thp->init.func(thp->init.argc, thp->init.argv);
  thread_end();
}

static kz_thread_id_t thread_run(kz_func_t func, char *name, int priority,
                                 int stacksize, int argc, char *argv[]) {
  int i;
  kz_thread *thp;
  uint32_t *sp;
  extern char userstack;
  static char *thread_stack = &userstack;

  for (i = 0; i < THREAD_NUM; ++i) {
    thp = &threads[i];
    if (!thp->init.func)
      break;
  }
  if (i == THREAD_NUM) {
    puts("ERR: no more threads");
    return -1;
  }

  memset(thp, 0, sizeof(*thp));

  strcpy(thp->name, name);
  thp->next = NULL;
  thp->priority = priority;
  thp->flags = 0;

  thp->init.func = func;
  thp->init.argc = argc;
  thp->init.argv = argv;

  // TODO スタック不足のチェック
  memset(thread_stack, 0, stacksize);
  thread_stack += stacksize;

  thp->stack = thread_stack;

  // スタックの初期化
  sp = (uint32_t *)thp->stack;
  *(--sp) = (uint32_t)thread_end; // 本来は不要

  // スレッドの優先度が0の場合には、割込み禁止スレッドとする
  *(--sp) = ((uint32_t)(priority ? 0 : 0xc0) << 24) // 上位1byte CCR
          | (uint32_t)thread_init; // 下位3byte プログラムカウンタ

  *(--sp) = 0; // ER6
  *(--sp) = 0; // ER5
  *(--sp) = 0; // ER4
  *(--sp) = 0; // ER3
  *(--sp) = 0; // ER2
  *(--sp) = 0; // ER1

  *(--sp) = (uint32_t)thp;  // ER0 thread_initの第一引数

  thp->context.sp = (uint32_t)sp;

  putcurrent();

  current = thp;
  putcurrent();

  return (kz_thread_id_t)current;
}

static int thread_exit(void) {
  /**
   * TODO 本来はスタックも解放して再利用すべきだが省略
   */
  puts(current->name);
  puts(" EXIT.\n");
  memset(current, 0, sizeof(*current));
  return 0;
}

static int thread_wait(void) {
  putcurrent();
  return 0;
}

static int thread_sleep(void) {
  // レディキューから外されたままなので、スケジューリングされなくなる
  return 0;
}

static int thread_wakeup(kz_thread_id_t id) {
  // 呼び出し元のスレッドをレディキューに戻す
  putcurrent();

  // 指定されたスレッドもレディキューに戻す
  current = (kz_thread *)id;
  putcurrent();

  return 0;
}

static kz_thread_id_t thread_getid(void) {
  putcurrent();
  return (kz_thread_id_t) current;
}

// 優先度変更
static int thread_chpri(int priority) {
  int old = current->priority;
  if (priority >= 0) {
    current->priority = priority;
  }
  putcurrent();

  return old;
}

static void *thread_kmalloc(int size) {
  putcurrent();
  return kzmem_alloc(size);
}

static int thread_kmfree(void *p) {
  kzmem_free(p);
  putcurrent();
  return 0;
}

static int sendmsg(kz_msgbox *mboxp, kz_thread *thp, int size, char *p) {
  kz_msgbuf *mp;

  mp = (kz_msgbuf *)kzmem_alloc(sizeof(*mp));
  if (mp == NULL) {
    kz_sysdown();
  }
  mp->next = NULL;
  mp->sender = thp;
  mp->param.size = size;
  mp->param.p = p;

  if (mboxp->tail) {
    mboxp->tail->next = mp;
  } else {
    mboxp->head = mp;
  }
  mboxp->tail = mp;
}

static void recvmsg(kz_msgbox *mboxp) {
  kz_msgbuf *mp;
  kz_syscall_param_t *p;

  mp = mboxp->head;
  mboxp->head = mp->next;
  if (mboxp->head == NULL) {
    mboxp->tail = NULL;
  }
  mp->next = NULL;

  p = mboxp->receiver->syscall.param;
  p->un.recv.ret = (kz_thread_id_t)mp->sender;
  if (p->un.recv.sizep) {
    *(p->un.recv.sizep) = mp->param.size;
  }
  if (p->un.recv.pp) {
    *(p->un.recv.pp) = mp->param.p;
  }

  mboxp->receiver = NULL;

  kzmem_free(mp);
}

static int thread_send(kz_msgbox_id_t id, int size, char *p) {
  kz_msgbox *mboxp = &msgboxes[id];

  putcurrent();
  sendmsg(mboxp, current, size, p);

  if (mboxp->receiver) {
    current = mboxp->receiver;
    recvmsg(mboxp);
    putcurrent();
  }

  return size;
}

static kz_thread_id_t thread_recv(kz_msgbox_id_t id, int *sizep, char **p) {
  kz_msgbox *mboxp = &msgboxes[id];

  if (mboxp->receiver) {
    puts("ERR: receiver is exists.\n");
    kz_sysdown();
  }

  mboxp->receiver = current;

  if (mboxp->head == NULL) {
    return -1;
  }

  recvmsg(mboxp);
  putcurrent();
  
  return current->syscall.param->un.recv.ret;
}

static int thread_setintr(softvec_type_t type, kz_handler_t handler) {
  static void thread_intr(softvec_type_t type, unsigned long sp);

  softvec_setintr(type, thread_intr);

  handlers[type] = handler;
  putcurrent();

  return 0;
}

void call_run(kz_syscall_param_t *p) {
  p->un.run.ret = thread_run(p->un.run.func, p->un.run.name,
                             p->un.run.priority, p->un.run.stacksize,
                             p->un.run.argc, p->un.run.argv);
}

void call_exit(kz_syscall_param_t *p) {
  thread_exit();
}

void call_wait(kz_syscall_param_t *p) {
  p->un.wait.ret = thread_wait();
}

void call_sleep(kz_syscall_param_t *p) {
  p->un.sleep.ret = thread_sleep();
}

void call_wakeup(kz_syscall_param_t *p) {
  p->un.wakeup.ret = thread_wakeup(p->un.wakeup.id);
}

void call_getid(kz_syscall_param_t *p) {
  p->un.getid.ret = thread_getid();
}

void call_chpri(kz_syscall_param_t *p) {
  p->un.chpri.ret = thread_chpri(p->un.chpri.priority);
}

void call_kmalloc(kz_syscall_param_t *p) {
  p->un.kmalloc.ret = thread_kmalloc(p->un.kmalloc.size);
}

void call_kmfree(kz_syscall_param_t *p) {
  p->un.kmfree.ret = thread_kmfree(p->un.kmfree.p);
}

void call_send(kz_syscall_param_t *p) {
  p->un.send.ret = thread_send(p->un.send.id,
                              p->un.send.size, p->un.send.p);
}

void call_recv(kz_syscall_param_t *p) {
  p->un.recv.ret = thread_recv(p->un.recv.id,
                              p->un.recv.sizep, p->un.recv.pp);
}

void call_setintr(kz_syscall_param_t *p) {
  p->un.setintr.ret = thread_setintr(p->un.setintr.type,
                                    p->un.setintr.handler);
}

static void call_functions(kz_syscall_type_t type, kz_syscall_param_t *p) {
  kz_syscall_func_t f = syscall_funcs[type];
  if (f) {
    f(p);
  } else {
    puts("ERR: unknown syscall");
  }
}

static void syscall_proc(kz_syscall_type_t type, kz_syscall_param_t *p) {
  /*
   * 呼び出し元のスレッドをレディーキューから外してから、システムコールを実行する。
   * システムコールを呼び出したスレッドをそのまま継続動作させたい場合は
   * 処理内部でputcurrent()を行う必要がある
   */
  getcurrent();
  call_functions(type, p);
}

static void srvcall_proc(kz_syscall_type_t type, kz_syscall_param_t *p) {
  // thread_intrvec()内部の割込みハンドラの延長で呼ばれる限りでは、
  // 呼出し後のthread_intrvec()でスケジューリング処理が行われ、
  // currentは再設定される。
  current = NULL;
  call_functions(type, p);
}

static void schedule(void) {
  // puts(">> schedule <<\n");

  int i;
  for (i = 0; i < PRIORITY_NUM; i++) {
    if (readyque[i].head) {
      current = readyque[i].head;
      return;
    }
  }

  // no threads
  kz_sysdown();
}

static void syscall_intr(void) {
  syscall_proc(current->syscall.type, current->syscall.param);
}

static void softerr_intr(void) {
  puts(current->name);
  puts(" DOWN by softerr intr.\n");
  getcurrent();
  thread_exit();
}

static void thread_intr(softvec_type_t type, unsigned long sp) {
  current->context.sp = sp;

  if (handlers[type]) {
    handlers[type]();
  }

  schedule();

  dispatch(&current->context);
}

void kz_start(kz_func_t func, char *name, int priority, int stacksize,
              int argc, char *argv[]) {

  kzmem_init();
  current = NULL;

  memset(readyque, 0, sizeof(readyque));
  memset(threads, 0, sizeof(threads));
  memset(handlers, 0, sizeof(handlers));
  memset(syscall_funcs, 0, sizeof(syscall_funcs));
  memset(msgboxes, 0, sizeof(msgboxes));

  // システムコールの設定
  syscall_funcs[KZ_SYSCALL_TYPE_RUN] = call_run;
  syscall_funcs[KZ_SYSCALL_TYPE_EXIT] = call_exit;
  syscall_funcs[KZ_SYSCALL_TYPE_WAIT] = call_wait;
  syscall_funcs[KZ_SYSCALL_TYPE_SLEEP] = call_sleep;
  syscall_funcs[KZ_SYSCALL_TYPE_WAKEUP] = call_wakeup;
  syscall_funcs[KZ_SYSCALL_TYPE_GETID] = call_getid;
  syscall_funcs[KZ_SYSCALL_TYPE_CHPRI] = call_chpri;
  syscall_funcs[KZ_SYSCALL_TYPE_KMALLOC] = call_kmalloc;
  syscall_funcs[KZ_SYSCALL_TYPE_KMFREE] = call_kmfree;
  syscall_funcs[KZ_SYSCALL_TYPE_SEND] = call_send;
  syscall_funcs[KZ_SYSCALL_TYPE_RECV] = call_recv;
  syscall_funcs[KZ_SYSCALL_TYPE_SETINTR] = call_setintr;

  // 割込みハンドラの設定
  thread_setintr(SOFTVEC_TYPE_SYSCALL, syscall_intr);
  thread_setintr(SOFTVEC_TYPE_SOFTERR, softerr_intr);

  current = (kz_thread *)thread_run(func, name, priority, stacksize, argc, argv);

  // 最初のスレッドを起動
  dispatch(&current->context);

  puts("ERR: no reach point 2");
}

void kz_sysdown(void) {
  puts("system error!\n");
  while (1) {
    asm volatile ("sleep");
  }
}

void kz_syscall(kz_syscall_type_t type, kz_syscall_param_t *param) {
  current->syscall.type = type; // システムコール番号
  current->syscall.param = param;
  asm volatile ("trapa #0"); // トラップ割込み発行
}

void kz_srvcall(kz_syscall_type_t type, kz_syscall_param_t *param) {
  srvcall_proc(type, param);
}

