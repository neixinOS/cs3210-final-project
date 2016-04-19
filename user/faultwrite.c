// buggy program - faults with a write to location zero

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int a[100];
	char *test = "Hello everyone, my name is Caesar. Nice to meet you!\n";
	cprintf("\n");
	cprintf("********************************\n");
	cprintf("Start Initialize the RAID Disk\n");
	cprintf("\n");
	sys_raid_init();
	int i;
	int j = 0;
	// for (i = 0; i < 100; i++) 
	// 	a[i] = i;
	while (test[j]) {
		a[j] = test[j];
		j++;
	}
	cprintf("Saving contents on RAID Disk\n");
	cprintf("\n");
	sys_raid_add(j, a);
	// sys_raid_change(1, 1, 4);
	// sys_raid_change(0, 0, 1);
	
	// sys_raid_change(1, 24, 4);
	int sectorNum = 12;
	assert(sectorNum < j);
	sys_raid_change(1, sectorNum, 91);
	sys_raid_check();
	return;
}
