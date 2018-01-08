#include <inc/lib.h>

void umain(int argc, char **argv)
{
	binaryname = "type";
	int result = 0;

	if (argc != 2)
	{
		cprintf("Wrong arguments!!!\n");
	}
	else 
	{
		result = know_type(argv[1]);
	}
	if (result == FTYPE_REG)
		cprintf("Type of file is REG\n");
	else if (result == FTYPE_DIR)
		cprintf("Type of file is DIR\n");
	else if (result == FTYPE_FIFO)
		cprintf("Type of file is FIFO\n");
	else
		cprintf("Error: unknown type!!!\n");
}