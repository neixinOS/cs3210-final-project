#include <inc/lib.h>

#define PROT_READ 0x0002       
#define PROT_WRITE 0x0004 
#define MAP_SHARED 0x0002 

void
umain(int argc, char **argv)
{
  int fd, n, r;
  char *s;
  char rdbuf[512+1];
  char wrbuf[512+1] = "CHALLENGE! memory-mapped file!\n";

  cprintf("mmap: open \n");
  if ((fd = open("/mmapfile", O_RDWR|O_CREAT)) < 0)
    panic("mmap: open /mmapfile: %e", fd);

  cprintf("mmap: read\n");
  if ((r = read(fd, rdbuf, sizeof(rdbuf))) < 0)
    panic("mmap: read /mmapfile: %e", fd);

  cprintf("before mmap()\n----\n%s----\n", rdbuf);

  cprintf("[%08x] mmap: mmap \n", sys_getenvid());
  if ((s = (char *)mmap(NULL, sizeof(wrbuf), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0)) < 0)
    panic("mmap: mmap /mmapfile: %e", -E_INVAL);

  cprintf("memory-mmapped after mmap()\n===\n%s===\n", s);
  
  cprintf("[%08x] mmap: strcpy(mmap'return_address, write_buffer)\n", sys_getenvid(), wrbuf); 
  strcpy(s, wrbuf);
  
  cprintf("memory-mmapped after srtcpy()\n===\n%s===\n", s);

  cprintf("mmap: munmap \n");
  if ((r = munmap(s, sizeof(wrbuf))) < 0)
    panic("mmap: munmap /mmapfile: %e", -E_INVAL);

  cprintf("mmap: close \n");
  if ((r = close(fd)) < 0)
    panic("mmap: close /mmapfile: %e", r);

  cprintf("mmap: open\n");
  if ((fd = open("/mmapfile", O_RDONLY)) < 0)
    panic("mmap: open /mmapfile: %e", fd);

  cprintf("mmap: read\n");
  if ((r = read(fd, rdbuf, sizeof(rdbuf))) < 0)
    panic("mmap: read /mmapfile: %e", fd);

  cprintf("after mmap()\n----\n%s----\n", rdbuf);

  cprintf("mmap: close \n");
  if ((r = close(fd)) < 0)
    panic("mmap: close /mmapfile: %e", r);
}
