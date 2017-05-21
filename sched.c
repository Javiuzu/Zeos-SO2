/*
 * sched.c - initializes struct for task 0 anda task 1
 */

#include <sched.h>
#include <mm.h>
#include <io.h>
//Afegim include utils per a que la crida get_ticks funcioni
#include <utils.h>

extern struct list_head freequeue;
extern struct list_head readyqueue;
extern struct list_head blockedqueue;
extern struct list_head keyboardqueue;

int lastPID = 2;
int quantum;
struct task_struct *idle_task;

/**
 * Container for the Task array and 2 additional pages (the first and the last one)
 * to protect against out of bound accesses.
 */
union task_union protected_tasks[NR_TASKS+2]
  __attribute__((__section__(".data.task")));

union task_union *task = &protected_tasks[1]; /* == union task_union task[NR_TASKS] */


struct task_struct *list_head_to_task_struct(struct list_head *l)
{
  return list_entry( l, struct task_struct, list);
}


extern struct list_head blocked;


/* get_DIR - Returns the Page Directory address for task 't' */
page_table_entry * get_DIR (struct task_struct *t) 
{
	return t->dir_pages_baseAddr;
}

/* get_PT - Returns the Page Table address for task 't' */
page_table_entry * get_PT (struct task_struct *t) 
{
	return (page_table_entry *)(((unsigned int)(t->dir_pages_baseAddr->bits.pbase_addr))<<12);
}


int allocate_DIR(struct task_struct *t) 
{
	int pos;

	pos = ((int)t-(int)task)/sizeof(union task_union);

	t->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos]; 

	return 1;
}

int allocate_clone(struct task_struct *t, struct task_struct *clone)
{
        int pos;

        pos = ((int)t-(int)task)/sizeof(union task_union);

        clone->dir_pages_baseAddr = (page_table_entry*) &dir_pages[pos];

        return 1;


}
void cpu_idle(void)
{
	//Per provar task_switch fer un print	
	//printc_xy(0, 0, 'C');

	__asm__ __volatile__("sti": : :"memory");

	while(1)
	{
	;
	}
}

void init_freequeue()
{	
  INIT_LIST_HEAD(&freequeue);
  for(int i = 0; i < NR_TASKS; ++i) {
 	list_add(&(task[i].task.list), &freequeue);
  }
}



void init_idle (void)
{
  struct list_head *first = list_first(&freequeue);
  struct task_struct *task_available = list_head_to_task_struct(first);
  task_available->PID = 0;
  task_available->stats.total_trans = 0;
  task_available->ticks = 10;
  allocate_DIR(task_available);
  union task_union *union_available = (union task_union *)task_available;
  union_available->stack[200] = (unsigned long)cpu_idle;
  union_available->stack[199] = 0;
  union_available->task.kernel_esp = (int)&union_available->stack[199];
  idle_task = task_available;

  list_del(first);
 

}

void init_task1(void)
{
  struct list_head *first = list_first(&freequeue);
  struct task_struct *task_available = list_head_to_task_struct(first);
  task_available->PID = 1;
  task_available->ticks = 10;
  allocate_DIR(task_available);
  set_user_pages(task_available);
  union task_union *union_available = (union task_union *)task_available;	
  tss.esp0 = (int)&union_available->stack[1024];
  task_available->stats.total_trans = 0;
  set_cr3(get_DIR(task_available));

  list_del(first);
}

void inner_task_switch(union task_union *new) 
{
	tss.esp0 = (int)&new->stack[1024];
	set_cr3(get_DIR(&new->task));
	int ret;
	__asm__ __volatile__(
	  "movl %%ebp, %0;"
	  : "=r" (ret)); 	
	current()->kernel_esp = ret;
	ret = new->task.kernel_esp;
	__asm__ __volatile__(
	 "movl %0, %%esp;"
	  "popl %%ebp;"
          "ret"
	  : //no return
	  : "r" (ret));
}

void task_switch(union task_union *new) 
{
	__asm__ __volatile__(
	   "pushl %esi;"
	   "pushl %edi;"
	   "pushl %ebx;"
	);
    inner_task_switch(new);
	__asm__ __volatile__(
	   "popl %ebx;"
	   "popl %edi;"
	   "popl %esi;"	
	);
}



void init_sched(){
  init_freequeue();
  INIT_LIST_HEAD(&readyqueue);
  INIT_LIST_HEAD(&blockedqueue);
  INIT_LIST_HEAD(&keyboardqueue);
}

struct task_struct* current()
{
  int ret_value;
  
  __asm__ __volatile__(
  	"movl %%esp, %0"
	: "=g" (ret_value)
  );
  return (struct task_struct*)(ret_value&0xfffff000);
}

void sched_next_rr() {

  
  struct task_struct *t;
  //Si la readyqueue esta buida agafam el process idle

  if (!list_empty(&readyqueue)) { 
	struct list_head  *next = list_first(&readyqueue);
	t = list_head_to_task_struct(next);
	list_del(next);
  }
  else t = idle_task;

  //Actualizem les dades del seguent process
  unsigned long current_ticks = get_ticks();
  t->stats.ready_ticks += current_ticks - t->stats.elapsed_total_ticks;
  t->stats.elapsed_total_ticks = get_ticks();
  quantum = get_quantum(t);
  t->stats.remaining_ticks = get_quantum(t);
  ++(t->stats.total_trans);
  
  //Començem el switch al seguent process

  task_switch((union task_union *)t);
   
}

void update_process_state_rr(struct task_struct *t, struct list_head *dst_queue) {

  unsigned long current_ticks = get_ticks();

  struct task_struct *currentProcess = current();
  currentProcess->stats.system_ticks += current_ticks - currentProcess->stats.elapsed_total_ticks;
  currentProcess->stats.elapsed_total_ticks = get_ticks();
  if (dst_queue != NULL) list_add_tail(&(t->list), dst_queue);
}

int needs_sched_rr() {
	return (quantum == 0);
}

void update_sched_data_rr() {
  --quantum;
  //--current()->stats.remaining_ticks;
  } 

int get_quantum(struct task_struct *t) {
  return t->ticks; 	
}

void set_quantum(struct task_struct *t, int new_quantum) {
  t->ticks = new_quantum;
}

void schedule() {
  update_sched_data_rr();
  if (needs_sched_rr()) {
     update_process_state_rr(current(), &readyqueue);
     sched_next_rr();   
  }
}

