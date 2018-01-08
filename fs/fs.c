#include <inc/string.h>
#include <inc/partition.h>

#include "fs.h"

#define debug 0
// --------------------------------------------------------------
// Super block
// --------------------------------------------------------------

// static const char * fs_files[] =
// {
// 	"newmotd",
// 	"motd",
// 	"bigfile",
// 	"script",
// 	"rmtest",
// 	"testshell.key",
// 	"testshell.sh",
// 	"init",
// 	"cat",
// 	"echo",
// 	"touch",
// 	"rm",
// 	"mkfifo",
// 	"type",
// 	"ls",
// 	"lsfd",
// 	"num",
// 	"sh",
// 	"testfdsharing",
// 	"testkbd",
// 	"testpipe",
// 	"testpteshare",
// 	"testshell",
// 	"hello",
// 	"date",
// 	"vdate",
// 	"abc",

// };

// Validate the file system super-block.
void
check_super(void)
{
	if (super->s_magic != FS_MAGIC)
		panic("bad file system magic number");

	if (super->s_nblocks > DISKSIZE/BLKSIZE)
		panic("file system is too large");

	cprintf("superblock is good\n");
}

// --------------------------------------------------------------
// Free block bitmap
// --------------------------------------------------------------

// Check to see if the block bitmap indicates that block 'blockno' is free.
// Return 1 if the block is free, 0 if not.
bool
block_is_free(uint32_t blockno)
{
	if (super == 0 || blockno >= super->s_nblocks)
		return 0;
	if (bitmap[blockno / 32] & (1 << (blockno % 32)))
		return 1;
	return 0;
}

// Mark a block free in the bitmap
void
free_block(uint32_t blockno)
{
	// Blockno zero is the null pointer of block numbers.
	if (blockno == 0)
		panic("attempt to free zero block");
	bitmap[blockno/32] |= 1<<(blockno%32);
}

// Search the bitmap for a free block and allocate it.  When you
// allocate a block, immediately flush the changed bitmap block
// to disk.
//
// Return block number allocated on success,
// -E_NO_DISK if we are out of blocks.
//
// Hint: use free_block as an example for manipulating the bitmap.
int
alloc_block(void)
{
	// The bitmap consists of one or more blocks.  A single bitmap block
	// contains the in-use bits for BLKBITSIZE blocks.  There are
	// super->s_nblocks blocks in the disk altogether.

	// LAB 10: Your code here.
	uint32_t blockno;
	for(blockno = 0; blockno < super->s_nblocks; blockno++)
	{
		if (block_is_free(blockno))
		{
			bitmap[blockno/32] &= ~(1<<(blockno%32));
			flush_block(bitmap);
			return blockno;
		}
	}
	return -E_NO_DISK;
}

// Validate the file system bitmap.
//
// Check that all reserved blocks -- 0, 1, and the bitmap blocks themselves --
// are all marked as in-use.
void
check_bitmap(void)
{
	uint32_t i;

	// Make sure all bitmap blocks are marked in-use
	for (i = 0; i * BLKBITSIZE < super->s_nblocks; i++)
		assert(!block_is_free(2+i));

	// Make sure the reserved and root blocks are marked in-use.
	assert(!block_is_free(0));
	assert(!block_is_free(1));

	cprintf("bitmap is good\n");
}

// --------------------------------------------------------------
// File system structures
// --------------------------------------------------------------



// Initialize the file system
void
fs_init(void)
{
	static_assert(sizeof(struct File) == 256);

	   // Find a JOS disk.  Use the second IDE disk (number 1) if availabl
	   if (ide_probe_disk1())
			   ide_set_disk(1);
	   else
			   ide_set_disk(0);
	bc_init();

	// Set "super" to point to the super block.
	super = diskaddr(1);
	check_super();

	// Set "bitmap" to the beginning of the first bitmap block.
	bitmap = diskaddr(2);
	check_bitmap();
	
}

// Find the disk block number slot for the 'filebno'th block in file 'f'.
// Set '*ppdiskbno' to point to that slot.
// The slot will be one of the f->f_direct[] entries,
// or an entry in the indirect block.
// When 'alloc' is set, this function will allocate an indirect block
// if necessary.
//
// Returns:
//	0 on success (but note that *ppdiskbno might equal 0).
//	-E_NOT_FOUND if the function needed to allocate an indirect block, but
//		alloc was 0.
//	-E_NO_DISK if there's no space on the disk for an indirect block.
//	-E_INVAL if filebno is out of range (it's >= NDIRECT + NINDIRECT).
//
// Analogy: This is like pgdir_walk for files.
// Hint: Don't forget to clear any block you allocate.
static int
file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
{
		// LAB 10: Your code here.
	if (filebno < NDIRECT) 
	{
			*ppdiskbno = &f->f_direct[filebno];
			return 0;
	}
	else if (filebno < NDIRECT + NINDIRECT) 
	{
		if (!f->f_indirect) 
		{
			if (!alloc) 
			{
				return -E_NOT_FOUND;
			}
					
			f->f_indirect = alloc_block();
			
			if (!f->f_indirect) 
			{
				return -E_NO_DISK;
			}

			memset(diskaddr(f->f_indirect), 0, BLKSIZE);
		}
		uint32_t *inderect_block = (uint32_t *)diskaddr(f->f_indirect);

		*ppdiskbno = &inderect_block[filebno - NDIRECT];
		return 0;
	}
	else return -E_INVAL;
}

// Set *blk to the address in memory where the filebno'th
// block of file 'f' would be mapped.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_NO_DISK if a block needed to be allocated but the disk is full.
//	-E_INVAL if filebno is out of range.
//
// Hint: Use file_block_walk and alloc_block.
int
file_get_block(struct File *f, uint32_t filebno, char **blk)
{
		// LAB 10: Your code here.
	uint32_t *pdiskbno;
	int r;
	r = file_block_walk(f, filebno, &pdiskbno, 1);
	if (r)
	{
		//	-E_INVAL if filebno is out of range.
		return r;
	}
	if (!*pdiskbno)
	{
		*pdiskbno = alloc_block();
	}
	if (!*pdiskbno)
	{
		return -E_NO_DISK;
	}
	*blk = (char *)diskaddr(*pdiskbno);
	return 0;
}

// Try to find a file named "name" in dir.  If so, set *file to it.
//
// Returns 0 and sets *file on success, < 0 on error.  Errors are:
//	-E_NOT_FOUND if the file is not found
static int
dir_lookup(struct File *dir, const char *name, struct File **file)
{
	int r;
	uint32_t i, j, nblock;
	char *blk;
	struct File *f;

	// Search dir for name.
	// We maintain the invariant that the size of a directory-file
	// is always a multiple of the file system's block size.
	assert((dir->f_size % BLKSIZE) == 0);
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
			if (strcmp(f[j].f_name, name) == 0) {
				*file = &f[j];
				return 0;
			}
	}
	return -E_NOT_FOUND;
}

// Set *file to point at a free File structure in dir.  The caller is
// responsible for filling in the File fields.
static int
dir_alloc_file(struct File *dir, struct File **file)
{
	int r;
	uint32_t nblock, i, j;
	char *blk;
	struct File *f;
	
	if (debug)
		cprintf("%s     %d\n", dir->f_name, dir->f_size);
	
	assert((dir->f_size % BLKSIZE) == 0);
	nblock = dir->f_size / BLKSIZE;
	for (i = 0; i < nblock; i++) {
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
			if (f[j].f_name[0] == '\0') {
				*file = &f[j];
				return 0;
			}
	}
	dir->f_size += BLKSIZE;
	
	if (debug)
		cprintf("%s     %d\n", dir->f_name, dir->f_size);
	
	if ((r = file_get_block(dir, i, &blk)) < 0)
		return r;
	f = (struct File*) blk;
	*file = &f[0];
	return 0;
}

// Remove a block from file f.  If it's not there, just silently succeed.
// Returns 0 on success, < 0 on error.
static int
file_free_block(struct File *f, uint32_t filebno)
{
	int r;
	uint32_t *ptr;

	if ((r = file_block_walk(f, filebno, &ptr, 0)) < 0)
		return r;
	if (*ptr) {
		free_block(*ptr);
		*ptr = 0;
	}
	return 0;
}

static void file_truncate_blocks(struct File *f, off_t newsize);


static int
dir_num_file(struct File *dir)
{
	int result = 0, r;
	uint32_t nblock, i, j;
	char *blk;
	struct File *f;
	nblock = dir->f_size / BLKSIZE;

	for(i = 0; i < nblock; i++)
	{
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		f = (struct File *)blk;
		for(j = 0; j < BLKFILES; j++)
		{
			if (f[j].f_name[0] == '\0')
				return result;
			result++;
		}
	}
	return result;
}

static int 
dir_remove_file(struct File *dir, struct File *f)
{
	int r;
	uint32_t nblock, i = 0, j = 0;
	char *blk;
	struct File *tmp  = NULL;
	struct File *next_tmp;
	int files_in_dir = 0;

	int num_struct = 0, num_block = 0, num_files = 0;

	if (debug)
		cprintf("%s     %d\n", dir->f_name, dir->f_size);
	assert((dir->f_size % BLKSIZE) == 0);
	
	files_in_dir = dir_num_file(dir);
	nblock = dir->f_size / BLKSIZE;
	if (debug)
		cprintf("BEFORE REMOVE IN DIR: NUMBER OF FILES %d\n", files_in_dir);

	for (i = 0; i < nblock; i++) 
	{
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		tmp = (struct File*) blk;
		for (j = 0; j < BLKFILES; j++)
		{
			if (!strcmp(tmp[j].f_name, f->f_name)) 
			{
				num_files = (BLKFILES - (j+1)) + (nblock -(i+1)) * BLKFILES;
				num_struct = j + 1;
				num_block = i + 1;
				//Очищаем место 
				if ((i <= 1) && (j <= 11))
				{
					cprintf("Attempt to remove important FS files\n");
					return -E_INVAL;
				}
				memset(&tmp[j], 0, sizeof(struct File));
				//Блок последний
				//cprintf("Current block = %d\n",i+1);
				if (i == nblock -1)
				{
					//Случай, когда структура находится в начале блока,
					//при этом блок последний,
					//тогда нужно очистить этот блок
					if ((j == 0) && (tmp[j+1].f_name[0] == '\0'))
					{
						file_truncate_blocks(dir, (nblock - 1) * BLKSIZE);
						dir->f_size -= BLKSIZE;
						return 0;
					}
					else if (j == BLKFILES -1)//Последняя в блоке структура, уже удалили, просто выходим
					{
						return 0;
					}
					else if (tmp[j+1].f_name[0] == '\0')//если последний блок, но НЕ последняя в блоке,
														//но последняя занятая
					{
						return 0; 
					}
					else
					{
						memmove(&tmp[j], &tmp[j+1], sizeof(struct File) * (BLKFILES - (j + 2)));
						return 0;
					}
				}
				else 
					goto move;
			}
		}
	}
move:
	//cprintf("%d\t%d\t%d\n", num_files, num_block, num_struct);
	//Перенос структур
	for(i = num_block; i < nblock; i++)
	{
		//двигаем оставишиеся
		if (num_struct != BLKFILES)
		{
			memmove(&tmp[j], &tmp[j+1], sizeof(struct File) * (BLKFILES - num_struct));
			num_struct = BLKFILES;
		}
		//берем следующий блок
		if ((r = file_get_block(dir, i, &blk)) < 0)
			return r;
		next_tmp = (struct File*) blk;
		//в последнюю структуру копируем первую следующего блока
		memmove(&tmp[BLKFILES-1], &next_tmp[0], sizeof(struct File));

		memmove(&next_tmp[0], &next_tmp[1], sizeof(struct File) * (BLKFILES - 1));
		tmp = next_tmp;
	}
	if (((files_in_dir - 1) % BLKFILES) == 0)
	{
		file_truncate_blocks(dir, (nblock - 1) * BLKSIZE);
		dir->f_size -= BLKSIZE;
	}

	return 0;
}

// Skip over slashes.
static const char*
skip_slash(const char *p)
{
	while (*p == '/')
		p++;
	return p;
}


// Evaluate a path name, starting at the root.
// On success, set *pf to the file we found
// and set *pdir to the directory the file is in.
// If we cannot find the file but find the directory
// it should be in, set *pdir and copy the final path
// element into lastelem.
static int
walk_path(const char *path, struct File **pdir, struct File **pf, char *lastelem)
{
	const char *p;
	char name[MAXNAMELEN];
	struct File *dir, *f;
	int r;

	// if (*path != '/')
	//	return -E_BAD_PATH;
	path = skip_slash(path);
	f = &super->s_root;
	dir = 0;
	name[0] = 0;

	if (pdir)
		*pdir = 0;
	*pf = 0;
	while (*path != '\0') {
		dir = f;
		p = path;
		while (*path != '/' && *path != '\0')
			path++;
		if (path - p >= MAXNAMELEN)
			return -E_BAD_PATH;
		memmove(name, p, path - p);
		name[path - p] = '\0';
		path = skip_slash(path);

		if (dir->f_type != FTYPE_DIR)
			return -E_NOT_FOUND;

		if ((r = dir_lookup(dir, name, &f)) < 0) {
			if (r == -E_NOT_FOUND && *path == '\0') {
				if (pdir)
					*pdir = dir;
				if (lastelem)
					strcpy(lastelem, name);
				*pf = 0;
			}
			return r;
		}
	}

	if (pdir)
		*pdir = dir;
	*pf = f;
	return 0;
}

// --------------------------------------------------------------
// File operations
// --------------------------------------------------------------

int 
file_type(const char *path)
{
	int r; 
	struct File *f;
	if ((r = walk_path(path, 0, &f, 0)) < 0)
	{
		if (debug)
			cprintf("Error in file_type: %i\n", r);
		return r;
	}
	if (f->f_type == FTYPE_REG)
		return FTYPE_REG;
	else if (f->f_type == FTYPE_DIR)
		return FTYPE_DIR;
	else if (f->f_type == FTYPE_FIFO)
		return FTYPE_FIFO;
	return r;
}

// Create "path".  On success set *pf to point at the file and return 0.
// On error return < 0.
int 
file_remove(const char *path)
{
	struct File *f, *dir;
	//Сначала находим структуры File самого файла и директрории, где он находится
	int r = walk_path(path, &dir, &f, 0);
	if (r < 0)
	{
		if (debug)
			cprintf("Error in file_remove: %i\n", r);
		return r;
	}
	//Очищаем все блоки, которые занимал файл своими данными
	file_set_size(f, 0);
	//Теперь нужно очистить память в блоке директории, 
	//которая была отведена под удаляемый struct File 
	dir_remove_file(dir, f);
	return 0;
}

int
file_create(const char *path, struct File **pf, uint32_t type)
{
	char name[MAXNAMELEN];
	int r;
	struct File *dir, *f;

	if ((r = walk_path(path, &dir, &f, name)) == 0)
		return -E_FILE_EXISTS;
	if (r != -E_NOT_FOUND || dir == 0)
		return r;
	if ((r = dir_alloc_file(dir, &f)) < 0)
		return r;

	if (type == FTYPE_FIFO)
		f->f_type = FTYPE_FIFO;
	strcpy(f->f_name, name);
	*pf = f;
	file_flush(dir);
	return 0;
}

// Open "path".  On success set *pf to point at the file and return 0.
// On error return < 0.
int
file_open(const char *path, struct File **pf)
{
	int r;
	r = walk_path(path, 0, pf, 0);
	return r;
}


// Read count bytes from f into buf, starting from seek position
// offset.  This meant to mimic the standard pread function.
// Returns the number of bytes read, < 0 on error.
ssize_t
file_read(struct File *f, void *buf, size_t count, off_t offset)
{
	int r, bn;
	off_t pos;
	char *blk;

	if (offset >= f->f_size)
		return 0;

	count = MIN(count, f->f_size - offset);

	for (pos = offset; pos < offset + count; ) {
		if ((r = file_get_block(f, pos / BLKSIZE, &blk)) < 0)
			return r;
		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
		memmove(buf, blk + pos % BLKSIZE, bn);
		pos += bn;
		buf += bn;
	}

	return count;
}


// Write count bytes from buf into f, starting at seek position
// offset.  This is meant to mimic the standard pwrite function.
// Extends the file if necessary.
// Returns the number of bytes written, < 0 on error.
int
file_write(struct File *f, const void *buf, size_t count, off_t offset)
{
	int r, bn;
	off_t pos;
	char *blk;

	// Extend file if necessary
	if (offset + count > f->f_size)
		if ((r = file_set_size(f, offset + count)) < 0)
			return r;

	for (pos = offset; pos < offset + count; ) {
		if ((r = file_get_block(f, pos / BLKSIZE, &blk)) < 0)
			return r;
		bn = MIN(BLKSIZE - pos % BLKSIZE, offset + count - pos);
		memmove(blk + pos % BLKSIZE, buf, bn);
		pos += bn;
		buf += bn;
	}

	return count;
}


// Remove any blocks currently used by file 'f',
// but not necessary for a file of size 'newsize'.
// For both the old and new sizes, figure out the number of blocks required,
// and then clear the blocks from new_nblocks to old_nblocks.
// If the new_nblocks is no more than NDIRECT, and the indirect block has
// been allocated (f->f_indirect != 0), then free the indirect block too.
// (Remember to clear the f->f_indirect pointer so you'll know
// whether it's valid!)
// Do not change f->f_size.
static void
file_truncate_blocks(struct File *f, off_t newsize)
{
	int r;
	uint32_t bno, old_nblocks, new_nblocks;

	old_nblocks = (f->f_size + BLKSIZE - 1) / BLKSIZE;
	new_nblocks = (newsize + BLKSIZE - 1) / BLKSIZE;
	for (bno = new_nblocks; bno < old_nblocks; bno++)
		if ((r = file_free_block(f, bno)) < 0)
			cprintf("warning: file_free_block: %i", r);

	if (new_nblocks <= NDIRECT && f->f_indirect) {
		free_block(f->f_indirect);
		f->f_indirect = 0;
	}
}



// Set the size of file f, truncating or extending as necessary.
int
file_set_size(struct File *f, off_t newsize)
{
	if (f->f_size > newsize)
		file_truncate_blocks(f, newsize);
	f->f_size = newsize;
	flush_block(f);
	return 0;
}

// Flush the contents and metadata of file f out to disk.
// Loop over all the blocks in file.
// Translate the file block number into a disk block number
// and then check whether that disk block is dirty.  If so, write it out.
void
file_flush(struct File *f)
{
	int i;
	uint32_t *pdiskbno;

	for (i = 0; i < (f->f_size + BLKSIZE - 1) / BLKSIZE; i++) {
		if (file_block_walk(f, i, &pdiskbno, 0) < 0 ||
			pdiskbno == NULL || *pdiskbno == 0)
			continue;
		flush_block(diskaddr(*pdiskbno));
	}
	flush_block(f);
	if (f->f_indirect)
		flush_block(diskaddr(f->f_indirect));
}


// Sync the entire file system.  A big hammer.
void
fs_sync(void)
{
	int i;
	for (i = 1; i < super->s_nblocks; i++)
		flush_block(diskaddr(i));
}

