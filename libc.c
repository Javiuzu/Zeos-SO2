/*
 * libc.c 
 */

#include <libc.h>

#include <types.h>

int errno;

void perror() {
  if (errno == 9) {
    char errorMessage[] = "Write (check file descriptor parameter)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 13) {
    char errorMessage[] = "Write (check return value for a valid write)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 38) {
    char errorMessage[] = "CHECK SYSCALL NOT IMPLEMENTED";
    write(1, errorMessage, strlen(errorMessage)); 
  }
  if (errno == 11) {
    char errorMessage[] = "WRITE(check address of source buffer)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 10) {
    char errorMessage[] = "WRITE(check size parameter)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 30) {
    char errorMessage[] = "FORK(not enough process space)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 31) {
    char errorMessage[] = "FORK(not enough free frames)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 35) {
    char errorMessage[] = "GET_STATS(pid parameter does not exist)";
    write(1, errorMessage, strlen(errorMessage));
  }
  if (errno == 36) {
    char errorMessage[] = "GET_STATS(pid parameter is negative)";
    write (1, errorMessage, strlen(errorMessage));
  }


}

void itoa(int a, char *b)
{
  int i, i1;
  char c;
  
  if (a==0) { b[0]='0'; b[1]=0; return ;}
  
  i=0;
  while (a>0)
  {
    b[i]=(a%10)+'0';
    a=a/10;
    i++;
  }
  
  for (i1=0; i1<i/2; i1++)
  {
    c=b[i1];
    b[i1]=b[i-i1-1];
    b[i-i1-1]=c;
  }
  b[i]=0;
}

int strlen(char *a)
{
  int i;
  
  i=0;
  
  while (a[i]!=0) i++;
  
  return i;
}

int write(int fd, char *buffer, int size) {
    int ret = 0;
    __asm__ __volatile__ (
      "movl %1, %%ebx;"
      "movl %2, %%ecx;"
      "movl %3, %%edx;"
      "movl $4, %%eax;" 
//Mou els parametres a la posicio desitjada, començant per el primer paramatre per el registre ebx.
//El 4 indica la crida a sistema desitjada
      "int $0x80" 
//Carreguem la rutina 0x80 d'interrupcio
      :"=r"(ret) 
//Indiquem que el registre ret és un registre de outpout, on hi desarem el resultat
      :"b" (fd), "c" (buffer), "d" (size) ); 
//Indiquem els registres que s'han de fer servir, el b indica ebx (Trobat a la web de SO2document ensamblador)
    if (ret < 0) {
      errno = -ret;
	    return -1;
    }
    else  return ret;
}

int gettime() {
   int ret = 0;
   __asm__ __volatile__ (
	"movl $10, %%eax;"
	"int $0x80;"
	:"=r" (ret)
	: /*No input*/ );
   return ret;

}

int getpid() {
   int ret = 0;
   __asm__ __volatile__ (
	"movl $20, %%eax;"
	"int $0x80;"
	:"=r" (ret)
	: /*No input*/ );
   return ret;
}


int fork() {
   int ret = 0;
   __asm__ __volatile__ (
	"movl $2, %%eax;"
	"int $0x80;"
	:"=r" (ret)
	: /*No input*/ );
  if (ret < 0) {
    errno = -ret;
    return -1;
  }
   return ret;
}

void exit() {
  __asm__ __volatile__ (
    "movl $1, %eax;"
    "int $0x80;"
    );
}

int get_stats(int pid, struct stats *st) {
   int ret = 0;
   __asm__ __volatile__ (
	"movl %1, %%ebx;"
	"movl %2, %%ecx;"
	"movl $35, %%eax;"
	"int $0x80;"
	:"=r" (ret)
	:"b"(pid), "c"(st));
  if (ret < 0) {
    errno = -ret;
    return -1;
  }
  return ret;
}

int clone(void (*function)(void), void *stack) {
  int ret = 0;
  __asm__ __volatile__ (
  	"movl %1, %%ebx;"
  	"movl %2, %%ecx;"
  	"movl $19, %%eax;"
  	"int $0x80;"
  	:"=r" (ret)
  	:"b"(function), "c"(stack));
  if (ret < 0) {
  	errno = -ret;
  	return -1;
  }
  return ret;	
}

int sem_init (int n_sem, unsigned int value) {
  int ret = 0;
  __asm__ __volatile__ (
  	"movl %1, %%ebx;"
  	"movl %2, %%ecx;"
  	"movl $21, %%eax;"
  	"int $0x80;"
  	:"=r" (ret)
  	:"b"(n_sem), "c"(value));
  if (ret < 0) {
  	errno =-ret;
  	return -1;
  }
  return ret;
}

int sem_wait(int n_sem) {
   int ret = 0;
   __asm__ __volatile__ (
   	"movl %1, %%ebx;"
   	"movl $22, %%eax;"
   	"int $0x80;"
   	:"=r" (ret)
   	:"b"(n_sem));
   if (ret < 0) {
  	errno =-ret;
  	return -1;
  }
  return ret;
}

int sem_signal(int n_sem) {
  int ret = 0;
   __asm__ __volatile__ (
   	"movl %1, %%ebx;"
   	"movl $23, %%eax;"
   	"int $0x80;"
   	:"=r" (ret)
   	:"b"(n_sem));
   if (ret < 0) {
  	errno =-ret;
  	return -1;
  }
  return ret;
}

int sem_destroy(int n_sem) {
  int ret = 0;
  __asm__ __volatile__ (
   	"movl %1, %%ebx;"
   	"movl $24, %%eax;"
   	"int $0x80;"
   	:"=r" (ret)
   	:"b"(n_sem));
   if (ret < 0) {
  	errno =-ret;
  	return -1;
  }
  return ret;
}

int read(int fd, char *buf, int count) {
   int ret = 0;
   __asm__ __volatile__ (
	"movl %1, %%ebx;"
	"movl %2, %%ecx;"
	"movl %3, %%edx;"
	"movl $5, %%eax;"
	"int $0x80;"
	:"=r" (ret)
	:"b"(fd), "c" (buf), "d" (count));
   if (ret < 0) {
	errno = -ret;
	return -1;
   }
   return ret;
}

void *sbrk (int increment) {
   int ret = 0;
   __asm__ __volatile__ (
	"movl %1, %%ebx;"
	"movl $6, %%eax;"
	"int $0x80;"
	:"=r" (ret)
	: "b"(increment));
   if (ret < 0) {
	errno = -ret;
	return (void *)-1;
  }
  return (void *)ret;
}

