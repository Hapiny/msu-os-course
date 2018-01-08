#include <inc/lib.h>

void umain(int argc, char **argv)
{
	binaryname = "touch";
	if (argc != 2)
	{
		printf("Wrong argumetns!!!\n");
	}
	else 
	{
		int fd;
		fd = open(argv[1], O_CREAT|O_EXCL);
		if (fd < 0)
		{
			printf("Can`t create file.\n");
			if (fd == -E_FILE_EXISTS)
			{
				printf("File already exists\n");
			}
		}
		else
		{
			close(fd);
			return;
		}
	}
}