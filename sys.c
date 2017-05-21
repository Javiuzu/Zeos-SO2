/*
 * sys.c - Syscalls implementation
 */
#include <devices.h>

#include <utils.h>

#include <io.h>

#include <mm.h>

#include <mm_address.h>

#include <sched.h>

#include <system.h>

#define LECTURA 0
#define ESCRIPTURA 1

extern int lastPID;
extern int dir_stack[NR_TASKS];

int last_read;
int inici, fin;

char read_buffer[32];
	 
int check_fd(int fd, int permissions)
{
  if (fd!=1) return -9; /*EBADF*/
  if (permissions!=ESCRIPTURA) return -13; /*EACCES*/
  return 0;
}


void user_stats(void) {
	unsigned long current_ticks = get_ticks();
	current()->stats.user_ticks += current_ticks - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
}

void system_stats(void) {
	unsigned long current_ticks = get_ticks();
	current()->stats.system_ticks += current_ticks - current()->stats.elapsed_total_ticks;
	current()->stats.elapsed_total_ticks = get_ticks();
}

int sys_ni_syscall()
{
	return -38; /*ENOSYS*/
}

int sys_getpid()
{
	return current()->PID;
}

int ret_from_fork() {
	return 0;
}

int sys_fork()
{
  int PID=-1;

  // creates the child process
  //Punt A
  if (list_empty(&freequeue)) return -30; /*EAGAIN*/
  struct list_head *first = list_first(&freequeue);
  struct task_struct *task_available = list_head_to_task_struct(first);
  list_del(first);
  //Punt B
  struct task_struct *curr= current();
  union task_union *newt = (union task_union *)task_available;
  union task_union *currentProcess = (union task_union *)curr;
  copy_data(currentProcess, newt, sizeof(union task_union));
  //Punt C
  allocate_DIR(task_available);
  //Punt D
  int i, j;
  unsigned int freeFrames[NUM_PAG_DATA];
  for (i = 0; i < NUM_PAG_DATA; ++i) {
  	freeFrames[i] = alloc_frame();
  	if (freeFrames[i] == -1) {
		for (j = i; j >= 0; --j) free_frame(freeFrames[j]);
		return -31 ; /*ENOMEM*/
  	}
  }
  //Punt E
  page_table_entry *table_child= get_PT(task_available);
  page_table_entry *table_parent = get_PT(&currentProcess->task);

//ASSIGNAR CODI
  unsigned int  logic_page_child[NUM_PAG_CODE];
  logic_page_child[0] = L_USER_START;
  for (i = 1; i < NUM_PAG_CODE; ++i) {
      	logic_page_child[i] = logic_page_child[i-1] + PAGE_SIZE;
  }
  for (i = 0; i < NUM_PAG_CODE; ++i) {
        logic_page_child[i] = PH_PAGE(logic_page_child[i]);
  }
  for (i = 0; i < NUM_PAG_CODE; ++i) {
	//L'espai lògic que ocupa el codi del pare i del fill es el mateix
	//per tant, al get frame podem mirar el frame de la adreça logica
	//logic_page_child[i]
	int frame_code = get_frame (table_parent, logic_page_child[i]);
      	set_ss_pag(table_child,logic_page_child[i],frame_code);
  }

//ASSIGNAR DATA+STACK

  //Espai logic extra pel pare
  unsigned int  logic_page[NUM_PAG_DATA];
  logic_page[0] = L_USER_START + NUM_PAG_CODE*PAGE_SIZE + NUM_PAG_DATA*PAGE_SIZE;
  for (i = 1; i < NUM_PAG_DATA; ++i) {
      	logic_page[i] = logic_page[i-1] + PAGE_SIZE;
  }
  //SHIFTAR
  for (i = 0; i < NUM_PAG_DATA; ++i) {
	logic_page[i] = PH_PAGE(logic_page[i]);
  }
  //ASSIGNAR logic/fisic del pare
  for (i = 0; i < NUM_PAG_DATA; ++i) {
	set_ss_pag(table_parent,logic_page[i],freeFrames[i]);
  }

  //Copia el data+Stack del pare
  copy_data((void *)L_USER_START + NUM_PAG_CODE*PAGE_SIZE,(void *) L_USER_START + NUM_PAG_CODE*PAGE_SIZE + NUM_PAG_DATA*PAGE_SIZE, NUM_PAG_DATA*PAGE_SIZE);

  //Espai logic pel fill

  logic_page_child[0] = L_USER_START + NUM_PAG_CODE*PAGE_SIZE;
  for (i = 1; i < NUM_PAG_DATA; ++i) {
        logic_page_child[i] = logic_page_child[i-1] + PAGE_SIZE;
  }
  //SHIFTAR
  for (i = 0; i < NUM_PAG_DATA; ++i) {
        logic_page_child[i] = PH_PAGE(logic_page_child[i]);
  }
  //ASSIGNAR espai logic/fisic a la copia del data+stack
  for (i = 0; i < NUM_PAG_DATA; ++i) {
        set_ss_pag(table_child,logic_page_child[i],freeFrames[i]);
  }
  //DESASSIGNAR espai logic/fisic del pare de la copia del data+stack
  for (i = 0; i < NUM_PAG_DATA; ++i) {
        del_ss_pag(table_parent,logic_page[i]);
  }


  //ASSIGNACIO DEL HEAP
  newt->task.heap_base = NUM_PAG_DATA + (NUM_PAG_CODE + L_USER_START) * PAGE_SIZE;;
  int heap_used_pages = currentProcess->task.heap_pages;
  newt->task.heap_pages = heap_used_pages;
  newt->task.heap_address = currentProcess->task.heap_address;
  unsigned int logic_page_heap[heap_used_pages];
  logic_page_heap[0] = NUM_PAG_DATA * PAGE_SIZE + (NUM_PAG_CODE + L_USER_START) * PAGE_SIZE;
  unsigned int framesHeap[heap_used_pages];
  for (int i = 0; i < heap_used_pages; ++i) {
 	framesHeap[i] = alloc_frame();
  	if (framesHeap[i] == -1) {
		for (j = i; j >= 0; --j) free_frame(framesHeap[j]);
		return -31 ; /*ENOMEM*/
  	}
  }
  /*HEAP DEL PARE*/
  unsigned int heap_page[heap_used_pages];
  heap_page[0] = NUM_PAG_DATA*PAGE_SIZE + (NUM_PAG_CODE + L_USER_START) * PAGE_SIZE;
  
  for (i = 1; i < heap_used_pages; ++i) {
	heap_page[i] = heap_page[i-1] + PAGE_SIZE;
  }
  for (i = 0; i < heap_used_pages; ++i) {
  	heap_page[i] = PH_PAGE(heap_page[i]);
  }
  
   for (i = 0; i < NUM_PAG_DATA; ++i) {
	set_ss_pag(table_parent,heap_page[i],framesHeap[i]);
  }
  
  
  /*HEAP DEL FILL */
  for (i = 1; i < heap_used_pages; ++i) {
    logic_page_heap[i] = logic_page_heap[i-1] + PAGE_SIZE;
  }
  for (i = 0; i < heap_used_pages; ++i) {
    logic_page_heap[i] = PH_PAGE(logic_page_heap[i]);
  }
  for (i = 0; i < NUM_PAG_DATA; ++i) {
        set_ss_pag(table_child,logic_page_heap[i],framesHeap[i]);
  }
  copy_data(newt->task.heap_base, newt->task.heap_base, heap_used_pages*PAGE_SIZE);
  
  
  for (i = 0; i < NUM_PAG_DATA; ++i) {
        del_ss_pag(table_parent,heap_page[i]);
  }

  set_cr3(get_DIR(curr));

  //Punt F
  PID = lastPID;
  newt->task.PID = PID; 
  ++lastPID;
  set_quantum(&newt->task, currentProcess->task.ticks);
  //Assignacio dels ticks del process fill i inicialitzacio d'estadistiques necessaries
  newt->task.ticks = currentProcess->task.ticks;
  newt->task.stats.total_trans = 0;


  //Punt G
  int tmp = ((int)task_available-(int)task)/sizeof(union task_union);
  dir_stack[tmp]++;

  //Punt H (si i el ret_from_fork es fer un return 0)
  int pos = KERNEL_STACK_SIZE - 1 -11 - 5; //@handler - SAVE ALL - Flags/instrucciones/pila usuario
  newt->stack[pos] = ret_from_fork;
  newt->stack[pos-1] = 0;
  newt->task.kernel_esp = (unsigned long)&newt->stack[pos-1];
  

  //Punt I
  //Actualitzem el valor de els tics que es ficara a ready abans de ficar a la cua
  newt->task.stats.elapsed_total_ticks = get_ticks();
  list_add_tail(&newt->task.list ,&readyqueue);
  
  //Punt J 
  return PID;
}

void sys_exit()
{  
  int i;
  struct task_struct *exitProcess = current();  
  page_table_entry *table_exit= get_PT(exitProcess);
  list_add_tail(&exitProcess->list, &freequeue);
  
  int tmp = ((int)exitProcess-(int)task)/sizeof(union task_union);
  dir_stack[tmp]--;

//CODE
  unsigned int  logic_code[NUM_PAG_CODE];
  logic_code[0] = L_USER_START;
  for (i = 1; i < NUM_PAG_CODE; ++i) {
        logic_code[i] = logic_code[i-1] + PAGE_SIZE;
  }

  for (i = 0; i < NUM_PAG_CODE; ++i) {
        logic_code[i] = PH_PAGE(logic_code[i]);
  }

  for (i = 0; i < NUM_PAG_CODE; ++i) {
        int frame_code = get_frame (table_exit, logic_code[i]);
        del_ss_pag(table_exit,logic_code[i]);
	if (dir_stack[tmp] == 0) free_frame(frame_code);

  }

//DATA+STACK
  unsigned int  logic_data[NUM_PAG_DATA];
  logic_data[0] = L_USER_START + NUM_PAG_CODE*PAGE_SIZE + NUM_PAG_DATA*PAGE_SIZE;
  for (i = 1; i < NUM_PAG_DATA; ++i) {
        logic_data[i] = logic_data[i-1] + PAGE_SIZE;
  }

  for (i = 0; i < NUM_PAG_DATA; ++i) {
        logic_data[i] = PH_PAGE(logic_data[i]);
  }

  for (i = 0; i < NUM_PAG_DATA; ++i) {
	int frame_data = get_frame(table_exit, logic_data[i]);
        del_ss_pag(table_exit, logic_data[i]);
        if (dir_stack[tmp] == 0) free_frame(frame_data);
  }

  
}

int sys_getstats(int pid, struct stats *st) {
  //Nomes tenim dos estats per el que busquem nomes a ready o al current
  if (pid < 0) return -36;
  if (st == NULL) return -1;
  if (!access_ok(VERIFY_WRITE, st, sizeof(struct stats))) return -1; 
  if (pid == current()->PID) {
    current()->stats.remaining_ticks = quantum;
    copy_to_user(&current()->stats, st, sizeof(struct stats));
    return 0;
  }
    int i;
    for (i=0; i<NR_TASKS; i++) {
    	if (task[i].task.PID==pid) {
      		task[i].task.stats.remaining_ticks=quantum;
      		copy_to_user(&(task[i].task.stats), st, sizeof(struct stats));
      		return 0;
    		}
   }
   return -35;
}

//funcio per escriure
int sys_write(int fd, char *buffer, int size) {
	int esEscriptura = check_fd(fd, ESCRIPTURA);
	//return -9;
	if (esEscriptura != 0) return esEscriptura;
	else if(buffer == NULL) return -1;
	else if(size < 0) return -10;
	char buff[10];
	int numEscritura = 0;
	while (size > 10) {
		copy_from_user(buffer, buff,10);
		numEscritura += sys_write_console(buff, 10);
		buffer +=10;
		size -= 10;
	}
	unsigned long s = (long)size;
	copy_from_user(buffer, buff, s);
	return numEscritura + sys_write_console(buff, size);
}

//funcio que dona el temps
int sys_gettime() {
      return zeos_ticks;
}

int sys_clone(void (*function)(void), void *stack) {
 int PID = -1;

  // creates the child process
  //PCB lliure
  if (list_empty(&freequeue)) return -30; /*EAGAIN*/
  struct list_head *first = list_first(&freequeue);
  struct task_struct *task_available = list_head_to_task_struct(first);
  list_del(first);
  //Passar a task union
  struct task_struct *curr= current();
  union task_union *newt = (union task_union *)task_available;
  union task_union *currentProcess = (union task_union *)curr;
  copy_data(currentProcess, newt, sizeof(union task_union));
  
  //Allocate clon
  allocate_clone(curr,task_available);
  

  PID = lastPID;
  newt->task.PID = PID;
  ++lastPID;
  set_quantum(&newt->task, currentProcess->task.ticks);
  //Assignacio dels ticks del process fill i inicialitzacio d'estadistiques necessaries
  newt->task.ticks = currentProcess->task.ticks;
  newt->task.stats.total_trans = 0;

  int tmp = ((int)curr-(int)task)/sizeof(union task_union);
  dir_stack[tmp]++;

  //Funció a executar
  int pos = KERNEL_STACK_SIZE - 1 -11 - 5; //@handler - SAVE ALL - Flags/instrucciones/pila usuario
  newt->stack[pos] = ret_from_fork;
  newt->stack[pos-1] = 0;
  newt->task.kernel_esp = (unsigned long)&newt->stack[pos-1];
  
  newt->stack[KERNEL_STACK_SIZE -5] = function;
  newt->stack[KERNEL_STACK_SIZE -1] = stack;


  //Actualitzem el valor de els tics que es ficara a ready abans de ficar a la cua
  newt->task.stats.elapsed_total_ticks = get_ticks();
  list_add_tail(&newt->task.list ,&readyqueue);

  return PID;
	
}

int sys_sem_init (int n_sem, unsigned int value) {
	if (n_sem > 20 || n_sem < 0) return -1; //Error no semaphor valid
	if (semaphores[n_sem].PID_owner != -1) return -38; //Error semaphor ja ocupat
	semaphores[n_sem].PID_owner = current()->PID;
	semaphores[n_sem].value = value;
	INIT_LIST_HEAD(&semaphores[n_sem].blocked_queue);
	return 0;
}

int sys_sem_wait(int n_sem) {
	if (n_sem > 20 || n_sem < 0) return -22;
	if (semaphores[n_sem].PID_owner == -1) return -1;
	if (semaphores[n_sem].value <= 0) {
		update_process_state_rr(current(), &semaphores[n_sem].blocked_queue);
		sched_next_rr();
	}
	else semaphores[n_sem].value--;
	return 0;
}

int sys_sem_signal(int n_sem) {
	if (n_sem > 20 || n_sem < 0) return -22; //Error no semaphor valid
        if (semaphores[n_sem].PID_owner != current()->PID) return -1; //Error semaphor ja ocupat
	if (list_empty(&semaphores[n_sem].blocked_queue)) 
		semaphores[n_sem].value++;
	else {
		struct list_head *first = list_first(&semaphores[n_sem].blocked_queue);
		struct task_struct *unblocked = list_head_to_task_struct(first);
		list_del(first);
		update_process_state_rr(unblocked, &readyqueue);
	}
	return 0;	
}

int sys_sem_destroy(int n_sem) {
	if (n_sem > 20 || n_sem < 0) return -22;
	if (semaphores[n_sem].PID_owner != current()->PID) return -1;
	semaphores[n_sem].PID_owner = -1;
	if (list_empty(&semaphores[n_sem].blocked_queue)) return 0;
	while (!list_empty(&semaphores[n_sem].blocked_queue)) {
		struct list_head *first = list_first(&semaphores[n_sem].blocked_queue);
		struct task_struct *unblocked = list_head_to_task_struct(first);
		list_del(first);
		update_process_state_rr(unblocked, &readyqueue);
	}
	return -1;	
}

int sys_read_keyboard(int fd, char *buf, int count) {
	int esLectura = check_fd(fd, LECTURA);
	if (esLectura != 0) return -9;
	if (buf == NULL) return -14;
	if (count < 0) return -10;
	if (!list_empty(&keyboardqueue)) {
		update_process_state_rr(current(), &keyboardqueue);
	}
	else {
		int tmp;
		if (inici > fin) tmp = 32 - (inici-fin);
		else tmp = fin - inici;

		if(tmp >= count) {
			int res = 0;
			if(inici < fin) res = copy_to_user(&read_buffer[inici], buf, count);
			else {
				res += copy_to_user(&read_buffer[inici],buf, 32-inici);
				res += copy_to_user(&read_buffer[0],buf, count - (32-inici));	
			}		
			current()->toRead -= count;	
			inici += count;
			if (inici >= 32) inici = inici-32;
			return res;			
		}
		else if (((inici - fin == 1) || (inici == 0 && fin == 31))) {
			if(inici < fin) copy_to_user(&read_buffer[inici], buf, tmp);
			else {
				copy_to_user(&read_buffer[inici],buf, 32-inici);
				copy_to_user(&read_buffer[0],buf,fin);	
			}	
			current()->toRead -= tmp;
			inici = 0;
			fin = 0;
			
			list_add(&current()->list, &keyboardqueue);
		}
		else {			
			current()->toRead = count;
			list_add(&current()->list, &keyboardqueue);
		}
		sched_next_rr();
	}
}

void *sys_sbrk(int increment) {
	struct task_struct *currentProcess = current();
	unsigned int dir_pcb = (unsigned int)currentProcess->heap_address;
	page_table_entry *currentPT = get_DIR(currentProcess);
	if (increment == 0) return (void *)dir_pcb;
	int pospagina = currentProcess->pos_page > + increment;
	if (increment > 0) {		
		if (pospagina> PAGE_SIZE) {
			int npags = pospagina / PAGE_SIZE;
			unsigned int log_page = PH_PAGE(dir_pcb);
			if (currentProcess->heap_pages + npags > TOTAL_PAGES) return (void *)-1;
			int i;
			for (i = 0; i < npags; ++i) {
				int freeFrame = alloc_frame();
				if (freeFrame == -1) {
				 free_frame(freeFrame);
				 return (void *)-1;
				}
				set_ss_pag(currentPT,log_page , freeFrame);
				++log_page;
			}
			currentProcess->pos_page = pospagina%PAGE_SIZE;
			currentProcess->heap_pages += npags;
		}
		else currentProcess->pos_page += increment;
		currentProcess->heap_address += increment;
		return currentProcess->heap_address;
	}
	else if (increment < 0) {
		if (pospagina < 0) {
			unsigned int log_page = PH_PAGE(dir_pcb);
			int npags = (-pospagina) / PAGE_SIZE;
			currentProcess->heap_address += increment;
 			if (currentProcess->heap_address < currentProcess->heap_base) return (void *)-1;
			for (int i = 0; i < npags; ++i) {
				unsigned int frame = get_frame(currentPT,log_page);
				free_frame(frame);
				--log_page;
			}
			currentProcess->pos_page = (-pospagina)%PAGE_SIZE;				
		}
		else currentProcess->pos_page = (-pospagina)%PAGE_SIZE;
		set_cr3(currentPT);
		return currentProcess->heap_address;
	}
}


		

  //unsigned int  logic_page_child_data[NUM_PAG_DATA];
