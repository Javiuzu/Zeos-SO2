# 1 "sys_call_table.S"
# 1 "<built-in>"
# 1 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 1 "<command-line>" 2
# 1 "sys_call_table.S"
# 1 "include/asm.h" 1
# 2 "sys_call_table.S" 2
# 1 "include/segment.h" 1
# 3 "sys_call_table.S" 2
# 33 "sys_call_table.S"
.globl sys_call_table; .type sys_call_table, @function; .align 0; sys_call_table:
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_fork
    .long sys_ni_syscall
    .long sys_write
    .long sys_read_keyboard
    .long sys_sbrk
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_gettime
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_clone
    .long sys_getpid
    .long sys_sem_init
    .long sys_sem_wait
    .long sys_sem_signal
    .long sys_sem_destroy
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_ni_syscall
    .long sys_getstats


.globl MAX_SYSCALL
MAX_SYSCALL = (. - sys_call_table)/4

.globl system_call_handler; .type system_call_handler, @function; .align 0; system_call_handler:
    pushl %gs; pushl %fs; pushl %es; pushl %ds; pushl %eax; pushl %ebp; pushl %edi; pushl %esi; pushl %edx; pushl %ecx; pushl %ebx; movl $0x18, %edx; movl %edx, %ds; movl %edx, %es
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
        popl %ebx; popl %ecx; popl %edx; popl %esi; popl %edi; popl %ebp; popl %eax; popl %ds; popl %es; popl %fs; popl %gs
        iret
