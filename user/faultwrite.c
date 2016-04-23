// test program for RAID

#include <inc/lib.h>

void
umain(int argc, char **argv)
{
	int a[100];
	cprintf("\n");
	cprintf("********************************\n");
	cprintf("Start Initialize the RAID Disk\n");
	cprintf("\n");
	sys_raid_init(NULL);
	int j = 0;
	cprintf("Saving contents on RAID Disk\n");
	cprintf("\n");
	sys_raid_add(j, a);

	// corrupt the data here
	sys_raid_change(1, 5, 91);
	sys_raid_change(1, 12, 42);

	// Since block 12 and 13 from same logical sector
	// if uncommet following line, sys_raid_check will print "sector 3 can't be repaired"
	//sys_raid_change(1, 13, 42);
	
	sys_raid_change(1, 22, 36);
	sys_raid_change(1, 25, 94);
	sys_raid_change(1, 29, 39);
	sys_raid_change(1, 34, 64);

	// check for the integrity
	sys_raid_check();

	return;
}
