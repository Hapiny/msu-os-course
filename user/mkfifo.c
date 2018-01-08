#include <inc/lib.h>

void umain(int argc, char **argv)
{
	binaryname = "mkfifo";
	int r = 0;

	if (argc != 2)
	{
		printf("Wrong argumetns!!!\n");
	}
	else 
	{
		if ((r = mkfifo(argv[1])) < 0)
		{
			cprintf("Error in mkfifo: %i\n", r);
		}
	}
}