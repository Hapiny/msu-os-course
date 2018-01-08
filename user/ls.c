#include <inc/lib.h>
#include <fs/fs.h>

bool with_block = false, with_type = false, with_pad = false; 
int file_count = 0;
int flag[256];

void lsdir(const char*, const char*);
void ls1(const char*, bool, off_t, const char*);

void
ls(const char *path, const char *prefix)
{
	int r;
	struct Stat st;
	if ((r = stat(path, &st)) < 0)
		panic("stat %s: %i", path, r);
	if (st.st_isdir && !flag['d'])
	{
		cprintf("===|| Directory: %s\n", path);
		cprintf("===|| Size of directory: %d\n", st.st_size);
		lsdir(path, prefix);
	}
	else
	{
		ls1(0, st.st_isdir, st.st_size, path);
	}

}

void
lsdir(const char *path, const char *prefix)
{
	int fd, n, total = 0;
	struct File f;

	if ((fd = open(path, O_RDONLY)) < 0)
		panic("open %s: %i", path, fd);
	while ((n = readn(fd, &f, sizeof f)) == sizeof f)
	{
		if (f.f_name[0])
		{
			if (with_type)
			{
				switch(f.f_type)
				{
					case FTYPE_REG:
						cprintf("|| TYPE || Regular file ");
						break;
					case FTYPE_DIR:
						cprintf("|| TYPE || Directory    ");
						break;
					case FTYPE_FIFO:
						cprintf("|| TYPE || FIFO file    ");
						break;
					default:
						cprintf("BAD TYPE");
						break;	
				}
			}
			ls1(prefix, f.f_type==FTYPE_DIR, f.f_size, f.f_name);
			if ((with_block) && (f.f_name[0] != '/'))
			{	
				int k;
				for(k = 0; k < 10; k++)
				{
					if (f.f_direct[k])
						cprintf("\tPointer to block %d = %d\n", k+1, f.f_direct[k]);
					else
						break;
				}
			}
			if ((with_pad) && (f.f_name[0] != '/'))
			{
				int k;
				for(k = 0; k < 19; k++)
					cprintf("%d ", f.f_pad[k]);
				cprintf("\n");
			}

		}
		total += n;
	}	
	if (n > 0)
		panic("short read in directory %s", path);
	if (n < 0)
		panic("error reading directory %s: %i", path, n);
	//cprintf("Size = %d\n", total);
}

void
ls1(const char *prefix, bool isdir, off_t size, const char *name)
{
	const char *sep;
	file_count++;
	if(flag['l'])
		printf("|| SIZE ||%6d ", size);
	if(prefix) {
		if (prefix[0] && prefix[strlen(prefix)-1] != '/')
			sep = "/";
		else
			sep = "";
		printf("%s%s", prefix, sep);
	}
	printf("|| NAME || %s ", name);
	if(flag['F'] && isdir)
		printf("/");
	printf("\n");
}

void
usage(void)
{
	printf("usage: ls [-dFl] [file...]\n");
	exit();
}

void
umain(int argc, char **argv)
{
	int i;
	struct Argstate args;
	file_count = 0;
	with_block = false;
	with_type = false;
	argstart(&argc, argv, &args);
	while ((i = argnext(&args)) >= 0)
		switch (i) {
		case 'd':
		case 'F':
		case 'l':
		case 'b':
		case 'p':
			flag[i]++;
			break;
		default:
			usage();
		}
	if (flag['p'])
		with_pad = true;
	if (flag['b'])
		with_block = true;
	if (flag['l'])
		with_type = true;
	if (argc == 1)
	{
		ls("/", "");
		cprintf("===========================\n");
		cprintf("Total number of files: %d\n", file_count);	
	}
	else {
		for (i = 1; i < argc; i++)
			ls(argv[i], argv[i]);
	}
}

