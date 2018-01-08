#include <inc/vsyscall.h>
#include <inc/lib.h>

bool first_run = true;

static inline int32_t
vsyscall(int num)
{
	// LAB 12: Your code here.
	if (num == VSYS_gettime)
	{
		if (first_run) 
		{
			sys_gettime();
			first_run = false;
		}
		return vsys[VSYS_gettime];
	}
	else 
	{
		panic("vsyscall not implemented");
		return -E_INVAL;
	}
}

int vsys_gettime(void)
{
	return vsyscall(VSYS_gettime);
}