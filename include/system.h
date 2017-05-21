/*
 * system.h - Capçalera del mòdul principal del sistema operatiu
 */

#ifndef __SYSTEM_H__
#define __SYSTEM_H__

#include <types.h>
#include <list.h>
struct  semaphore_struct {
	int PID_owner;
	int value;
	struct list_head blocked_queue;
};

extern unsigned int zeos_ticks;
extern TSS         tss;
extern Descriptor* gdt;
extern struct semaphore_struct semaphores[20];

#endif  /* __SYSTEM_H__ */
