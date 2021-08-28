#ifndef _CONSDRV_H_INCLUDED_
#define _CONSDRV_H_INCLUDED_

// console driver thread / system task
int consdrv_main(int argc, char *argv[]);

#define CONSDRV_DEVICE_NUM 1
#define CONSDRV_CMD_USE 'u'
#define CONSDRV_CMD_WRITE 'w'

#endif

