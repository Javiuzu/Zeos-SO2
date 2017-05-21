#include <libc.h>

char buff[24];

int pid;

int __attribute__ ((__section__(".text.main")))
  main(void)
{
    /* Next line, tries to move value 0 to CR3 register. This register is a privileged one, and so it will raise an exception */
     /* __asm__ __volatile__ ("mov %0, %%cr3"::"r" (0) ); */
  /*struct stats st;
  
  while (1)  {
	get_stats(getpid(), &st);
	char stats[100]; 
        itoa(st.user_ticks, stats);
	write(1, stats, strlen(stats));
	} */
  runjp();

  while(1) { }
}
