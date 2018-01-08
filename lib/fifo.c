#include <inc/lib.h>

#define debug 0

static ssize_t devfifo_read(struct Fd *fd, void *buf, size_t n);
static ssize_t devfifo_write(struct Fd *fd, const void *buf, size_t n);
static int devfifo_stat(struct Fd *fd, struct Stat *stat);
static int devfifo_close(struct Fd *fd);

struct Dev devfifo =
{
	.dev_id =	'i',
	.dev_name =	"fifo",
	.dev_read =	devfifo_read,
	.dev_write = devfifo_write,
	.dev_close = devfifo_close,
	.dev_stat = devfifo_stat,
};

#define FIFOBUFSIZ 32		// small to provoke races

struct Fifo 
{
	struct Fd * fd_fifo_read;  //pointer of fd for read from fifo
	struct Fd * fd_fifo_write; // pointer of fd for write to fifo
	off_t fifo_rpos;		// read position
	off_t fifo_wpos;		// write position
	uint8_t fifo_buf[FIFOBUFSIZ];	// data buffer
};

static ssize_t 
devfifo_read(struct Fd *fd, void *buf, size_t n)
{
	cprintf("\nAttempt to read from FIFO\n");
	return 0;
}


static ssize_t 
devfifo_write(struct Fd *fd, const void *buf, size_t n)
{
	cprintf("\nAttempt to read from FIFO\n");
	return 0;
}

static int 
devfifo_stat(struct Fd *fd, struct Stat *stat)
{
	struct Fifo *i = (struct Fifo*) fd2data(fd);
	strcpy(stat->st_name, "<fifo>");
	stat->st_size = i->fifo_wpos - i->fifo_rpos;
	stat->st_isdir = 0;
	stat->st_dev = &devpipe;
	return 0;
}

static int 
devfifo_close(struct Fd *fd)
{
	(void) sys_page_unmap(0, fd);
	return sys_page_unmap(0, fd2data(fd));
}


