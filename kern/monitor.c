// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>

#include <kern/trap.h>

#include <kern/pmap.h>


#define CMDBUF_SIZE 80 // enough for one VGA text line

uint32_t str2addr(char *str);
bool pa_con(uint32_t addr, uint32_t * value);

struct Command {
  const char *name;
  const char *desc;
  // return -1 to force monitor to exit
  int (*func)(int argc, char **argv, struct Trapframe * tf);
};

static struct Command commands[] = {
  { "help",      "Display this list of commands",        mon_help       },
  { "info-kern", "Display information about the kernel", mon_infokern   },
  { "showmappings", "Display physical page mappings and their permission bits", mon_showmappings },
  { "setmapping", "set, clear, or change the permissions of mapping", mon_setmapping },
  { "dumpcontents", "Dump the contents of a range of memory", mon_dumpcontents},
  { "pagedirectory", "show the page directory entry information", mon_pagedirectory},
};
#define NCOMMANDS (sizeof(commands)/sizeof(commands[0]))

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
  int i;

  for (i = 0; i < NCOMMANDS; i++)
    cprintf("%s - %s\n", commands[i].name, commands[i].desc);
  return 0;
}

int
mon_infokern(int argc, char **argv, struct Trapframe *tf)
{
  extern char _start[], entry[], etext[], edata[], end[];

  cprintf("Special kernel symbols:\n");
  cprintf("  _start                  %08x (phys)\n", _start);
  cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
  cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
  cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
  cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
  cprintf("Kernel executable memory footprint: %dKB\n",
          ROUNDUP(end - entry, 1024) / 1024);
  return 0;
}


int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
  // Your code here.
  uint32_t* ebp;
  uint32_t eip;
  ebp = (uint32_t *)read_ebp();
  cprintf("Stack backtrace:\n");
  while (ebp) {
    eip = *(ebp + 1);
    cprintf("  ebp %08x  eip %08x  args %08x %08x %08x %08x %08x\n", 
      ebp, eip, *(ebp + 2), *(ebp + 3), *(ebp + 4), *(ebp + 5), *(ebp + 6));
    struct Eipdebuginfo info;
    debuginfo_eip(eip, &info);
    cprintf("\t%s:%d: %.*s+%d\n", 
      info.eip_file, info.eip_line, info.eip_fn_namelen, info.eip_fn_name,
      eip-info.eip_fn_addr);
    ebp = (uint32_t *)*ebp;
  }
  return 0;
}



/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
  int argc;
  char *argv[MAXARGS];
  int i;

  // Parse the command buffer into whitespace-separated arguments
  argc = 0;
  argv[argc] = 0;
  while (1) {
    // gobble whitespace
    while (*buf && strchr(WHITESPACE, *buf))
      *buf++ = 0;
    if (*buf == 0)
      break;

    // save and scan past next arg
    if (argc == MAXARGS-1) {
      cprintf("Too many arguments (max %d)\n", MAXARGS);
      return 0;
    }
    argv[argc++] = buf;
    while (*buf && !strchr(WHITESPACE, *buf))
      buf++;
  }
  argv[argc] = 0;

  // Lookup and invoke the command
  if (argc == 0)
    return 0;
  for (i = 0; i < NCOMMANDS; i++)
    if (strcmp(argv[0], commands[i].name) == 0)
      return commands[i].func(argc, argv, tf);
  cprintf("Unknown command '%s'\n", argv[0]);
  return 0;
}

void
monitor(struct Trapframe *tf)
{
  char *buf;

  cprintf("Welcome to the JOS kernel monitor!\n");
  cprintf("Type 'help' for a list of commands.\n");
  cprintf("%m%s\n%m%s\n%m%s\n", 
    0x0100, "blue", 0x0200, "green", 0x0400, "red");

  if (tf != NULL)
    print_trapframe(tf);

  while (1) {
    buf = readline("K> ");
    if (buf != NULL)
      if (runcmd(buf, tf) < 0)
        break;
  }
}

/***** Implementations of extened kernel monitor commands *****/
uint32_t
str2addr(char *str)
{
  uint32_t va;
  va = 0;
  // skip 0x
  assert(*str == '0');
  str++;
  assert(*str == 'x');
  str++; 

  while (*str) {
    // a, b, c, d, e,f
    uint32_t cur;
    cur = 0;
    if (*str >= 'a' && *str <= 'f') {
      cur = *str - 'a' + 10;
    } else if (*str >= '0' && *str <= '9') {
      cur = *str - '0';
    }
    va = cur | (va << 4);
    str++;
  }

  return va;
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
  if (argc != 3) {
    cprintf("Command should be: showmappings [addr1] [addr2]\n");
    cprintf("Example: showmappings 0xf0000000 0xf0002000\n");
    cprintf("         physical page mappings from 0xf0000000 to 0xf0002000\n");
  } else {
    uint32_t begin;
    uint32_t end;
    begin = str2addr(argv[1]);
    end = str2addr(argv[2]);
    while (begin <= end) {
      pte_t *pte;
      pte = pgdir_walk(kern_pgdir, (void *)begin, 1);
      if (!pte) {
        panic("Allocation fails!\n");
        //return 0;
      }
      if (*pte & PTE_P) {
        cprintf("page: %x, PTE_P: %x, PTE_W: %x, PTE_U: %x\n", 
          begin, *pte & PTE_P, *pte & PTE_W, *pte & PTE_U);
      } else {
        cprintf("page: %x does not exist!\n", begin);
      }
      begin += PGSIZE;
    }
  }
  return 0;
}


int
mon_setmapping(int argc, char **argv, struct Trapframe *tf) 
{
  if (argc != 4) {
    cprintf("Command should be: setmapping [addr] [0|1] [P|W|U]\n");
    cprintf("Example: setmapping 0xf0000010 1 P\n");
    cprintf("         set the permission of PTE_P at 0xf0000010\n");
  } else {
    uint32_t va;
    uint32_t perm;
    pte_t *pte;
    va = str2addr(argv[1]);
    pte = pgdir_walk(kern_pgdir, (void *)va, 1);
    cprintf("1: page: %x, PTE_P: %x, PTE_W: %x, PTE_U: %x\n", 
          va, *pte & PTE_P, *pte & PTE_W, *pte & PTE_U);
    if (argv[3][0] == 'P') {
      perm = PTE_P;
    } else if (argv[3][0] == 'W') {
      perm = PTE_W;
    } else if (argv[3][0] == 'U') {
      perm = PTE_U;
    } else {
      cprintf("Invalid input!\n");
      return 0;
    }
    if (argv[2][0] == '0') {
      *pte = *pte & ~perm;
    } else if (argv[2][0] == '1') {
      *pte = *pte | perm;
    } else {
      cprintf("Invalid input!\n");
      return 0;
    }
    cprintf("2: page: %x, PTE_P: %x, PTE_W: %x, PTE_U: %x\n", 
          va, *pte & PTE_P, *pte & PTE_W, *pte & PTE_U);
  }
  return 0;
}
int 
mon_dumpcontents(int argc, char **argv, struct Trapframe *tf)
{
  if (argc != 4) {
    cprintf("Command should be: dumpcontents [v|p] [addr1] [addr2]\n");
    cprintf("Example: dumpcontents v 0xf0000000 0xf0000010\n");
    cprintf("         dump contents between virtual address 0xf0000000 and 0xf0000010\n");
  } else {
    uint32_t begin;
    uint32_t end;
    // make sure addrs are multiple of 4
    begin = ROUNDDOWN(str2addr(argv[2]), 4);
    end = ROUNDDOWN(str2addr(argv[3]), 4);
    // virtual address
    if (argv[1][0] == 'v') {
      pte_t * pte;
      while (begin < end) {
        pte = pgdir_walk(kern_pgdir, (void *)ROUNDDOWN(begin, PGSIZE), 0);
        if (pte && (*pte & PTE_P)) {
          cprintf("0x%08x\n", *((uint32_t *)begin));
        }
        begin += 4;
      }
    } else if (argv[1][0] == 'p') {
        // physical address
        uint32_t content;
        while (begin < end) {
          // get content at physical address
          if (pa_con(begin, &content)) {
            cprintf("0x%08x\n", content);
          } 
        begin += 4;
      }
    }
  }
  return 0;
}

bool
pa_con(uint32_t addr, uint32_t *content)
{
  if (addr >= PADDR(pages) && addr < PADDR(pages) + PTSIZE) {
    // PageInfo
    *content = *(uint32_t *)(UPAGES + (addr - PADDR(pages)));
    return true;
  }
  if (addr >= PADDR(bootstack) && addr < PADDR(bootstack) + KSTKSIZE) {
    // kernel stack
    *content = *(uint32_t *)(KSTACKTOP - KSTKSIZE + (addr - PADDR(bootstack)));
    return true;
  }
  if (addr < -KERNBASE) {
    // Other
    *content = *(uint32_t *)(addr + KERNBASE);
    return true;
  }
  // Not in virtual memory mapped, return false.
  return false;
}

int 
mon_pagedirectory(int argc, char **argv, struct Trapframe *tf)
{
    if (argc != 2) {
        cprintf("Command should be: pagedirectory [entrynumber]\n");
        cprintf("Example: pagedirectory 0x01\n");
        cprintf("         show kernel page directory[1] infomation \n");
    } else {
        uint32_t dir;
        dir = str2addr(argv[1]);
        if (dir < 0 || dir >= 1024) {
            panic("Invalid input, it should be in [0, 1024)\n");
        } else {
            cprintf("pgdir[%d] = 0x%08x\n", dir, (uint32_t)kern_pgdir[dir]);
        }
    }
    return 0;
}
