#include <asm.h>
#include <segment.h>

#define SAVE_ALL \
      pushl %gs; \
      pushl %fs; \
      pushl %es; \
      pushl %ds; \
      pushl %eax; \
      pushl %ebp; \
      pushl %edi; \
      pushl %esi; \
      pushl %edx; \
      pushl %ecx; \
      pushl %ebx; \
      movl $__KERNEL_DS, %edx;    \
      movl %edx, %ds;           \
      movl %edx, %es
#define RESTORE_ALL \
      popl %ebx; \
      popl %ecx; \
      popl %edx; \
      popl %esi; \
      popl %edi; \
      popl %ebp; \
      popl %eax; \
      popl %ds; \
      popl %es; \
      popl %fs; \
      popl %gs \


ENTRY (sys_call_table)
    .long sys_ni_syscall//0
    .long sys_ni_syscall//1
    .long sys_fork//2
    .long sys_ni_syscall//3
    .long sys_write//4
    .long sys_read_keyboard//5
    .long sys_sbrk//6
    .long sys_ni_syscall//7
    .long sys_ni_syscall//8
    .long sys_ni_syscall//9
    .long sys_gettime//10
    .long sys_ni_syscall//11
    .long sys_ni_syscall//12
    .long sys_ni_syscall//13
    .long sys_ni_syscall//14
    .long sys_ni_syscall//15
    .long sys_ni_syscall//16
    .long sys_ni_syscall//17
    .long sys_ni_syscall//18
    .long sys_clone//19
    .long sys_getpid//20
    .long sys_sem_init//21
    .long sys_sem_wait//22
    .long sys_sem_signal//23
    .long sys_sem_destroy//24
    .long sys_ni_syscall//25
    .long sys_ni_syscall//26
    .long sys_ni_syscall//27
    .long sys_ni_syscall//28
    .long sys_ni_syscall//29
    .long sys_ni_syscall//30
    .long sys_ni_syscall//31
    .long sys_ni_syscall//32
    .long sys_ni_syscall//33
    .long sys_ni_syscall//34
    .long sys_getstats//35


.globl MAX_SYSCALL
MAX_SYSCALL = (. - sys_call_table)/4

ENTRY (system_call_handler)
    SAVE_ALL
    pushl %eax;
    call user_stats;
    popl %eax;
    cmpl $0, %eax
    jl err
    cmpl $MAX_SYSCALL, %eax
    jg err
    call *sys_call_table(, %eax, 0x04)
    jmp fin
    err:
	movl $-38, %eax
    fin:
        movl %eax, 0x18(%esp)
        pushl %eax;
        call system_stats;
        popl %eax;
        RESTORE_ALL
        iret 
