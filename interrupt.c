/*
 * interrupt.c -
 */
#include <types.h>
#include <interrupt.h>
#include <segment.h>
#include <hardware.h>
#include <io.h>


#include <zeos_interrupt.h>

extern unsigned int zeos_ticks;
void keyboard_handler();
void timer_handler();
void system_call_handler();

Gate idt[IDT_ENTRIES];
Register    idtR;

int inici, fin;
int last_read;
extern char read_buffer[32];


char char_map[] =
{
  '\0','\0','1','2','3','4','5','6',
  '7','8','9','0','\'','¡','\0','\0',
  'q','w','e','r','t','y','u','i',
  'o','p','`','+','\0','\0','a','s',
  'd','f','g','h','j','k','l','ñ',
  '\0','º','\0','ç','z','x','c','v',
  'b','n','m',',','.','-','\0','*',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0','\0','\0','\0','\0','\0','7',
  '8','9','-','4','5','6','+','1',
  '2','3','0','\0','\0','\0','<','\0',
  '\0','\0','\0','\0','\0','\0','\0','\0',
  '\0','\0'
};

void setInterruptHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE INTERRUPTION GATE FLAGS:                          R1: pg. 5-11  */
  /* ***************************                                         */
  /* flags = x xx 0x110 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}

void setTrapHandler(int vector, void (*handler)(), int maxAccessibleFromPL)
{
  /***********************************************************************/
  /* THE TRAP GATE FLAGS:                                  R1: pg. 5-11  */
  /* ********************                                                */
  /* flags = x xx 0x111 000 ?????                                        */
  /*         |  |  |                                                     */
  /*         |  |   \ D = Size of gate: 1 = 32 bits; 0 = 16 bits         */
  /*         |   \ DPL = Num. higher PL from which it is accessible      */
  /*          \ P = Segment Present bit                                  */
  /***********************************************************************/
  Word flags = (Word)(maxAccessibleFromPL << 13);

  //flags |= 0x8F00;    /* P = 1, D = 1, Type = 1111 (Trap Gate) */
  /* Changed to 0x8e00 to convert it to an 'interrupt gate' and so
     the system calls will be thread-safe. */
  flags |= 0x8E00;    /* P = 1, D = 1, Type = 1110 (Interrupt Gate) */

  idt[vector].lowOffset       = lowWord((DWord)handler);
  idt[vector].segmentSelector = __KERNEL_CS;
  idt[vector].flags           = flags;
  idt[vector].highOffset      = highWord((DWord)handler);
}


void setIdt()
{
  /* Program interrups/exception service routines */
  idtR.base  = (DWord)idt;
  idtR.limit = IDT_ENTRIES * sizeof(Gate) - 1;
  
  set_handlers();
  setInterruptHandler(33, keyboard_handler, 0);
  setInterruptHandler(32, timer_handler, 0);
  setTrapHandler(0x80, system_call_handler, 3);

  /* ADD INITIALIZATION CODE FOR INTERRUPT VECTOR */

  set_idt_reg(&idtR);
}

void keyboard_routine() {
    unsigned short port = 0x60;
    unsigned char t = inb(port);
    unsigned char tmp = t >> 6;
    if (tmp == 0) {
	char c = char_map[t];
	if (c == '\0') printc_xy(0, 0, 'C');
	else printc_xy(0, 0, c);
	} 
}

void keyboard_read_routine() {
    

    if( ((inici - fin == 1) || (inici == 0 && fin == 31)) && list_empty(&keyboardqueue)) 
         return -1;
    
    unsigned short port = 0x60;
    unsigned char t = inb(port);
    unsigned char tmp = t >> 6;
    if (tmp == 0) {
	char c = char_map[t];
        if (c == '\0') read_buffer[fin] = 'C' ;
	else read_buffer[fin] = c;
	fin++;
	if (fin == 32) fin = 0;
    }
    
    int aux;
    if (inici > fin) aux = 32 - (inici-fin);
    else aux = fin - inici;

    struct list_head *first = list_first(&keyboardqueue);
    struct task_struct *curr = list_head_to_task_struct(first);	
    
    if ((inici - fin == 1) || (inici == 0 && fin == 31)) {
	list_del(first);
	update_process_state_rr(curr, &readyqueue);
    }

    if(aux == curr->toRead) {
	
	list_del(first);
	curr->toRead = -1;
	update_process_state_rr(curr, &readyqueue);

    }
}

void timer_routine() {
	++zeos_ticks;
	zeos_show_clock();
	schedule();
	//Provar task switch
        //union task_union *union_idle = (union task_union *)idle_task;
        //if(zeos_ticks == 2000) task_switch(union_idle);

	
}



