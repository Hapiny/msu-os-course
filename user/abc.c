#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int pfd[2], r = 0;
	if ((r = pipe(pfd)) < 0)
		cprintf("ERROR: %i\n", r);
	
	cprintf("%d  %d\n", pfd[0], pfd[1]);

	while(1)
		sys_yield();
	
	if ((r = spawnl("/lsfd", "lsfd", "-1")) < 0)
		cprintf("ERROR in spawn: %i\n", r);
	
	if (r)
		wait(r);
	
	cprintf("DONE!!!\n");
}

