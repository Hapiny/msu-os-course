#include <inc/lib.h>

void umain(int argc, char **argv)
{
	binaryname = "rm";
	if (argc != 2)
	{
		printf("Wrong argumetns!!!\n");
	}
	else 
	{
		int result = remove(argv[1]);
		if (result < 0)
		{
			printf("Can`t remove file.\n");
			if (result == -E_NOT_FOUND)
			{
				printf("File not exist\n");
			}
		}
		else
		{
			return;
		}
	}
}