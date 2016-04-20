/* See COPYRIGHT for copyright information. */

#include <inc/x86.h>
#include <inc/error.h>
#include <inc/string.h>
#include <inc/assert.h>

#include <kern/env.h>
#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/syscall.h>
#include <kern/console.h>
#include <kern/sched.h>
#include <kern/time.h>

#include <kern/e1000.h>

// Print a string to the system console.
// The string is exactly 'len' characters long.
// Destroys the environment on memory errors.
static void
sys_cputs(const char *s, size_t len)
{
  // Check that the user has permission to read memory [s, s+len).
  // Destroy the environment if not.

  // LAB 3: Your code here.
  user_mem_assert(curenv , (void *)s, len, PTE_U);

  // Print the string supplied by the user.
  cprintf("%.*s", len, s);
}

static int
sys_vcprintf(const char *fmt, va_list ap) {
  uint32_t len;
  for (len = 0; fmt[len] != '\0'; ++len);
  user_mem_assert(curenv , (void *)fmt, len, PTE_U);
  return vcprintf(fmt, ap);
}

// Read a character from the system console without blocking.
// Returns the character, or 0 if there is no input waiting.
static int
sys_cgetc(void)
{
  return cons_getc();
}

// Returns the current environment's envid.
static envid_t
sys_getenvid(void)
{
  return curenv->env_id;
}

// Destroy a given environment (possibly the currently running environment).
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
static int
sys_env_destroy(envid_t envid)
{
  int r;
  struct Env *e;

  if ((r = envid2env(envid, &e, 1)) < 0)
    return r;
  env_destroy(e);
  return 0;
}

// Deschedule current environment and pick a different one to run.
static void
sys_yield(void)
{
  sched_yield();
}

// Allocate a new environment.
// Returns envid of new environment, or < 0 on error.  Errors are:
//  -E_NO_FREE_ENV if no free environment is available.
//  -E_NO_MEM on memory exhaustion.
static envid_t
sys_exofork(void)
{
  // Create the new environment with env_alloc(), from kern/env.c.
  // It should be left as env_alloc created it, except that
  // status is set to ENV_NOT_RUNNABLE, and the register set is copied
  // from the current environment -- but tweaked so sys_exofork
  // will appear to return 0.

  // LAB 4: Your code here.
  // panic("sys_exofork not implemented");
  struct Env *e;
  int r;
  if ((r = env_alloc(&e, curenv->env_id)) < 0) {
    return r;
  }
  e->env_tf = curenv->env_tf;
  e->env_status = ENV_NOT_RUNNABLE;
  e->env_tf.tf_regs.reg_eax = 0;
  return e->env_id;
}

// Set envid's env_status to status, which must be ENV_RUNNABLE
// or ENV_NOT_RUNNABLE.
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
//  -E_INVAL if status is not a valid status for an environment.
static int
sys_env_set_status(envid_t envid, int status)
{
  // Hint: Use the 'envid2env' function from kern/env.c to translate an
  // envid to a struct Env.
  // You should set envid2env's third argument to 1, which will
  // check whether the current environment has permission to set
  // envid's status.

  // LAB 4: Your code here.
  // panic("sys_env_set_status not implemented");
  if (status != ENV_NOT_RUNNABLE && status != ENV_RUNNABLE) {
    return -E_INVAL;
  }
  struct Env *e; 
  int r;
  if ((r = envid2env(envid, &e, 1)) < 0) {
    return -E_BAD_ENV;
  }
  e->env_status = status;
  return 0;
}

// Set envid's trap frame to 'tf'.
// tf is modified to make sure that user environments always run at code
// protection level 3 (CPL 3) with interrupts enabled.
//
// Returns 0 on success, < 0 on error.  Errors are:
//	-E_BAD_ENV if environment envid doesn't currently exist,
//		or the caller doesn't have permission to change envid.
static int
sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
{
  // LAB 5: Your code here.
  // Remember to check whether the user has supplied us with a good
  // address!
  // panic("sys_env_set_trapframe not implemented");
  struct Env *e; 
  int r;
  if ((r = envid2env(envid, &e, 1)) < 0) {
    return -E_BAD_ENV;
  }
  user_mem_assert(e, tf, sizeof(struct Trapframe), PTE_U);
  e->env_tf = *tf;
  e->env_tf.tf_eflags |= FL_IF;
  e->env_tf.tf_cs = GD_UT | 3;
  return 0; 
}

// Set the page fault upcall for 'envid' by modifying the corresponding struct
// Env's 'env_pgfault_upcall' field.  When 'envid' causes a page fault, the
// kernel will push a fault record onto the exception stack, then branch to
// 'func'.
//
// Returns 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
static int
sys_env_set_pgfault_upcall(envid_t envid, void *func)
{
  // LAB 4: Your code here.
  //panic("sys_env_set_pgfault_upcall not implemented");
  struct  Env *e;
  int r;
  if ((r = envid2env(envid, &e, 1)) < 0) {
    return -E_BAD_ENV;
  }
  e->env_pgfault_upcall = func;
  return 0;
}

// Allocate a page of memory and map it at 'va' with permission
// 'perm' in the address space of 'envid'.
// The page's contents are set to 0.
// If a page is already mapped at 'va', that page is unmapped as a
// side effect.
//
// perm -- PTE_U | PTE_P must be set, PTE_AVAIL | PTE_W may or may not be set,
//         but no other bits may be set.  See PTE_SYSCALL in inc/mmu.h.
//
// Return 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
//  -E_INVAL if va >= UTOP, or va is not page-aligned.
//  -E_INVAL if perm is inappropriate (see above).
//  -E_NO_MEM if there's no memory to allocate the new page,
//    or to allocate any necessary page tables.
static int
sys_page_alloc(envid_t envid, void *va, int perm)
{
  // Hint: This function is a wrapper around page_alloc() and
  //   page_insert() from kern/pmap.c.
  //   Most of the new code you write should be to check the
  //   parameters for correctness.
  //   If page_insert() fails, remember to free the page you
  //   allocated!

  // LAB 4: Your code here.
  // panic("sys_page_alloc not implemented");
  struct Env *e; 
  int r;
  if ((r = envid2env(envid, &e, 1)) < 0) {
    return -E_BAD_ENV;
  }

  if (va >= (void *)UTOP) {
    return -E_INVAL;
  }

  if ((perm & (PTE_U|PTE_P)) != (PTE_U|PTE_P)) {
    return -E_INVAL;
  }

  struct PageInfo *pg;
  if (!(pg = page_alloc(ALLOC_ZERO))) {
    return -E_NO_MEM;
  }

  if ((r = page_insert(e->env_pgdir, pg, va, perm)) < 0) {
    page_free(pg);
    return -E_BAD_ENV;
  }
  // pg->pp_ref++;
  return 0;
}

// Map the page of memory at 'srcva' in srcenvid's address space
// at 'dstva' in dstenvid's address space with permission 'perm'.
// Perm has the same restrictions as in sys_page_alloc, except
// that it also must not grant write access to a read-only
// page.
//
// Return 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if srcenvid and/or dstenvid doesn't currently exist,
//    or the caller doesn't have permission to change one of them.
//  -E_INVAL if srcva >= UTOP or srcva is not page-aligned,
//    or dstva >= UTOP or dstva is not page-aligned.
//  -E_INVAL is srcva is not mapped in srcenvid's address space.
//  -E_INVAL if perm is inappropriate (see sys_page_alloc).
//  -E_INVAL if (perm & PTE_W), but srcva is read-only in srcenvid's
//    address space.
//  -E_NO_MEM if there's no memory to allocate any necessary page tables.
static int
sys_page_map(envid_t srcenvid, void *srcva,
             envid_t dstenvid, void *dstva, int perm)
{
  // Hint: This function is a wrapper around page_lookup() and
  //   page_insert() from kern/pmap.c.
  //   Again, most of the new code you write should be to check the
  //   parameters for correctness.
  //   Use the third argument to page_lookup() to
  //   check the current permissions on the page.

  // LAB 4: Your code here.
  // panic("sys_page_map not implemented");
  struct Env *se;
  struct Env *de;
  int r;
  if ((r = envid2env(srcenvid, &se, 1)) < 0) {
    return -E_BAD_ENV;
  }
  if ((r = envid2env(dstenvid, &de, 1)) < 0) {
    return -E_BAD_ENV;
  }

  if (srcva >= (void *)UTOP || ROUNDDOWN(srcva, PGSIZE) != srcva || 
    dstva >= (void*)UTOP || ROUNDDOWN(dstva,PGSIZE) != dstva) {
    return -E_INVAL;
  }


  pte_t *pte;
  struct PageInfo *pg;
  if (!(pg = page_lookup(se->env_pgdir, srcva, &pte))) {
    return -E_INVAL;
  }

  if ((perm & (PTE_U|PTE_P)) != (PTE_U|PTE_P)) {
    return -E_INVAL;
  }

  if ((perm & PTE_W) && !(*pte & PTE_W)) {
    return -E_INVAL;
  }

  if ((r = page_insert(de->env_pgdir, pg, dstva, perm)) < 0) {
    return -E_NO_MEM;
  }
  //pg->pp_ref++;
  return 0;
}

// Unmap the page of memory at 'va' in the address space of 'envid'.
// If no page is mapped, the function silently succeeds.
//
// Return 0 on success, < 0 on error.  Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist,
//    or the caller doesn't have permission to change envid.
//  -E_INVAL if va >= UTOP, or va is not page-aligned.
static int
sys_page_unmap(envid_t envid, void *va)
{
  // Hint: This function is a wrapper around page_remove().

  // LAB 4: Your code here.
  // panic("sys_page_unmap not implemented");
  // LAB 4: Your code here.
  struct Env *e;
  int r;
  if ((r = envid2env(envid, &e, 1)) < 0) {
    return -E_BAD_ENV;
  }
  page_remove(e->env_pgdir, va);
  if (va >= (void *)UTOP || ROUNDDOWN(va, PGSIZE) != va) {
    return -E_INVAL;
  }

  return 0;
}

// Try to send 'value' to the target env 'envid'.
// If srcva < UTOP, then also send page currently mapped at 'srcva',
// so that receiver gets a duplicate mapping of the same page.
//
// The send fails with a return value of -E_IPC_NOT_RECV if the
// target is not blocked, waiting for an IPC.
//
// The send also can fail for the other reasons listed below.
//
// Otherwise, the send succeeds, and the target's ipc fields are
// updated as follows:
//    env_ipc_recving is set to 0 to block future sends;
//    env_ipc_from is set to the sending envid;
//    env_ipc_value is set to the 'value' parameter;
//    env_ipc_perm is set to 'perm' if a page was transferred, 0 otherwise.
// The target environment is marked runnable again, returning 0
// from the paused sys_ipc_recv system call.  (Hint: does the
// sys_ipc_recv function ever actually return?)
//
// If the sender wants to send a page but the receiver isn't asking for one,
// then no page mapping is transferred, but no error occurs.
// The ipc only happens when no errors occur.
//
// Returns 0 on success, < 0 on error.
// Errors are:
//  -E_BAD_ENV if environment envid doesn't currently exist.
//    (No need to check permissions.)
//  -E_IPC_NOT_RECV if envid is not currently blocked in sys_ipc_recv,
//    or another environment managed to send first.
//  -E_INVAL if srcva < UTOP but srcva is not page-aligned.
//  -E_INVAL if srcva < UTOP and perm is inappropriate
//    (see sys_page_alloc).
//  -E_INVAL if srcva < UTOP but srcva is not mapped in the caller's
//    address space.
//  -E_INVAL if (perm & PTE_W), but srcva is read-only in the
//    current environment's address space.
//  -E_NO_MEM if there's not enough memory to map srcva in envid's
//    address space.
static int
sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
{
  // LAB 4: Your code here.
  // panic("sys_ipc_try_send not implemented");
  struct Env *e;
  int r;

  if ((r = envid2env(envid, &e, 0)) < 0) {
    return -E_BAD_ENV;
  }

  if (!e->env_ipc_recving) {
    return -E_IPC_NOT_RECV;
  }
  if (srcva < (void *)UTOP) {
    pte_t *pte;
    struct PageInfo *pg;
    if (!(pg = page_lookup(curenv->env_pgdir, srcva, &pte))) {
      return -E_INVAL;
    }

    if ((*pte & perm & 7) != (perm & 7)) {
      return -E_INVAL;
    }
    if ((perm & PTE_W) && !(*pte & PTE_W)) {
      return -E_INVAL;
    }
    if (srcva != ROUNDDOWN(srcva, PGSIZE)) {
      return -E_INVAL;
    }
    if (e->env_ipc_dstva < (void *)UTOP) {
      if ((r = page_insert(e->env_pgdir, pg, e->env_ipc_dstva, perm)) < 0) {
        return -E_NO_MEM;
      }
      e->env_ipc_perm = perm;
    }
  }

  e->env_ipc_from = curenv->env_id;
  e->env_status = ENV_RUNNABLE;
  e->env_ipc_recving = 0;
  e->env_ipc_value = value; 
  e->env_tf.tf_regs.reg_eax = 0;

  return 0;
}

// Block until a value is ready.  Record that you want to receive
// using the env_ipc_recving and env_ipc_dstva fields of struct Env,
// mark yourself not runnable, and then give up the CPU.
//
// If 'dstva' is < UTOP, then you are willing to receive a page of data.
// 'dstva' is the virtual address at which the sent page should be mapped.
//
// This function only returns on error, but the system call will eventually
// return 0 on success.
// Return < 0 on error.  Errors are:
//  -E_INVAL if dstva < UTOP but dstva is not page-aligned.
static int
sys_ipc_recv(void *dstva)
{
  // LAB 4: Your code here.
  // panic("sys_ipc_recv not implemented");
  if (dstva < (void *)UTOP) {
    if (dstva != ROUNDDOWN(dstva, PGSIZE)) {
      return -E_INVAL;
    }
  }

  curenv->env_status = ENV_NOT_RUNNABLE;
  curenv->env_ipc_dstva = dstva;
  curenv->env_ipc_recving = 1;

  sys_yield();
  return 0;
}

static int
sys_env_set_gpfault_upcall(envid_t envid, void *func)
{

  struct Env * env;
  int r = envid2env(envid, &env, 1);
  if (r < 0) return r;

  env->env_gpfault_upcall = func; 

  return 0;
}

static int
sys_env_set_divide0_upcall(envid_t envid, void *func)
{

  struct Env * env;
  int r = envid2env(envid, &env, 1);
  if (r < 0) return r;

  env->env_divide0_upcall = func; 

  return 0;
}

static int
sys_env_set_illegalop_upcall(envid_t envid, void *func)
{

  struct Env * env;
  int r = envid2env(envid, &env, 1);
  if (r < 0) return r;

  env->env_illegalop_upcall = func; 

  return 0;
}

// Return the current time.
static int
sys_time_msec(void)
{
  // LAB 6: Your code here.
  // panic("sys_time_msec not implemented");
  return time_msec();
}

// Add a system call that lets you transmit packets from user space
static int
sys_net_pkt_transmit(char *data, int length)
{
	if ((uintptr_t)data >= UTOP) {
		return -E_INVAL;
	}
	return e1000_pkt_transmit(data, length);
}

static int
sys_net_try_receive(char *data, int *len)
{
  if ((uintptr_t) data >= UTOP)
    return -E_INVAL;

  *len = e1000_pkt_receive(data);
  if (*len > 0)
    return 0;
  return *len;
}

static void printState() {
  struct My_Disk* cur_disk;
  int i;
  for (i = 0; i < 4; i++) {
    cur_disk = &raid_disks[i];
    while (cur_disk != NULL) {
      cprintf("%d", cur_disk->data);
      cur_disk = cur_disk->next;
    }
  }
}

static void disk_alloc() {
  int i;
  for (i = 0; i < 4; i++, now_raid_disk++) {
    user_raid_disk[i]->next = &raid_disks[now_raid_disk];
    user_raid_disk[i] = &raid_disks[now_raid_disk];
    user_raid_disk[i]->next = NULL;
    user_raid_disk[i]->data = 0;
    user_raid_disk[i]->now = 0;
    user_raid_disk[i]->dirty = false;
  }
  cprintf("~~~~~~~~~~~~~~~~~~%d~~~~~~~~~~~~~~~~~~~~ Disk alloc\n", now_raid_disk);
  return;
}

static void Xor() {
  cprintf("disk 0 %d\n", user_raid_disk[0]->data);
  cprintf("disk 1 %d\n", user_raid_disk[1]->data);
  cprintf("disk 2 %d\n", user_raid_disk[2]->data);
  user_raid_disk[3]->data = user_raid_disk[0]->data ^ user_raid_disk[1]->data ^ user_raid_disk[2]->data;

  cprintf("   The xor value is %d\n", user_raid_disk[3]->data);
  cprintf("\n");

  disk_alloc();
}

int gg[300];
int umark = 0;

static void sys_raid_init(uint32_t* a) {
  
  if (a == NULL)
  {
    // cprintf("piece of mess!!!!!!!!!\n");
    // cprintf("%d\n", umark);
    int i;
    for (i = 0; i < 4; i++, now_raid_disk++) {
      user_raid_disk[i] = &raid_disks[now_raid_disk];
      origin_raid_disk[i] = &raid_disks[now_raid_disk];
      user_raid_disk[i]->next = NULL;
      user_raid_disk[i]->data = 0;
      user_raid_disk[i]->now = 0;
      user_raid_disk[i]->dirty = false;
    }
    cprintf("~~~~~~~~~~~~~~~~~~%d~~~~~~~~~~~~~~~~~~~~ Disk alloc\n", now_raid_disk);
    now_raid_add = 0;
    return;
  } else {
    int j;
    char c;
    // cprintf("%d\n", sizeof(a) / sizeof(int));
    for (j = 0; j < 12; j++) {
      gg[umark] = a[j];
      c = a[j];
      cprintf("%c", c);
      umark++;
    }
    cprintf("\n");
    //cprintf("%d\n", umark);
  }
  // int i;
  // for (i = 0; i < 4; i++, now_raid_disk++) {
  //   user_raid_disk[i] = &raid_disks[now_raid_disk];
  //   origin_raid_disk[i] = &raid_disks[now_raid_disk];
  //   user_raid_disk[i]->next = NULL;
  //   user_raid_disk[i]->data = 0;
  //   user_raid_disk[i]->now = 0;
  //   user_raid_disk[i]->dirty = false;
  // }
  // cprintf("~~~~~~~~~~~~~~~~~~%d~~~~~~~~~~~~~~~~~~~~ Disk alloc\n", now_raid_disk);
  // now_raid_add = 0;
  // return;
}

static void sys_raid_add(int num, uint32_t* a) {

  int i, count;
  count = num;
  for (i = 0; i < num; i++) {
    // cprintf("Add %d to disk\n", a[i]);

    int tmp = a[i];
    cprintf("\tAdd %d to disk %d\n", tmp, now_raid_add);
    if (tmp)
    {
      user_raid_disk[now_raid_add]->data = tmp;
    } else {
      user_raid_disk[now_raid_add]->data = 0;
    }
    now_raid_add++;
    count--;
    if (count == 0) {
      // if (now_raid_add != 3)
      // {
      // }
      cprintf("Allocate for the last disk\n");
      Xor();
      break;
    }
    if (now_raid_add == 3) {
      now_raid_add = 0;
      cprintf("Write to parity disk\n");
      Xor();
    }

  }

  // count = umark;
  // for (i = 0; i < umark; ++i)
  // {
  //   int tmp = gg[i];
  //   cprintf("\tAdd %d to disk %d\n", tmp, now_raid_add);
  //   if (tmp)
  //   {
  //     user_raid_disk[now_raid_add]->data = tmp;
  //   } else {
  //     user_raid_disk[now_raid_add]->data = 0;
  //   }
  //   now_raid_add++;
  //   count--;
  //   if (count == 0) {
  //     // if (now_raid_add != 3)
  //     // {
  //     // }
  //     cprintf("Allocate for the last disk\n");
  //     Xor();
  //     break;
  //   }
  //   if (now_raid_add == 3) {
  //     now_raid_add = 0;
  //     cprintf("Write to parity disk\n");
  //     Xor();
  //   }
  // }

}

static void sys_raid_change(int onDisk, int num, int change) {
  int count = 0;
  struct My_Disk* cur_disk;

  // cprintf("\n");
  // cprintf("Before the change\n");

  // while (raid_disks[count].next != NULL) {
  //   cur_disk = &raid_disks[count];
  //   cprintf("Sector %d\n", count);
  //   cprintf("%d        ", cur_disk->data);
  //   cprintf("%d\n", cur_disk->dirty);
  //   count++;
  // }

  raid_disks[num].dirty = true;
  raid_disks[num].data = change;

  cprintf("\n");
  cprintf("After We Intentionally corrupt:\n");
  char c;
  while (raid_disks[count].next != NULL) {
    cur_disk = &raid_disks[count];
    c = cur_disk->data;
    if (count == 0 || (count - 3) % 4 != 0)
    {
      cprintf("%c", c);
    }
    count++;
  }

  return;

  // if (is_disk) {
  //   // num is the exact place in the physical memory
  //   // ie, need to be 0, 1, 2, (no 3), 4, 5, 6, (no 7)
  //   raid_disks[num].dirty = true;
  //   raid_disks[num].data = change;
  //   return;
  // }

  // //is_disk表明我是不是整块硬盘进行修改，而change为我进行修改的值，
  // //num在修改硬盘情况下，表示我修改的是第几块硬盘，否则表示我修改是数据第多少位。
  // struct My_Disk* tmp_disk[4];

  // int i;
  // for (i = 0; i < 4; i++) 
  //   tmp_disk[i] = origin_raid_disk[i];
  // for (;num > 32 * 4;) {
  //   num -= 32 * 4;
  //   for (i = 0; i < 4; i++)
  //     tmp_disk[i] = tmp_disk[i]->next;
  // }
  // int row = num / 3;
  // int col = num % 3;
  // // if (col >= 1) col += 3; else col += 2;
  // tmp_disk[col]->data &= ~(1 << row);
  // tmp_disk[col]->data |= change;
  // return;
}


struct My_Disk* tmp_disk[4];

static void sys_raid_check() {
  cprintf("\n");
  cprintf("Check for RAID Disk integrity\n");
  int sum_d = 0;
  int i;
  int j;
  for (i = 0; i < 4; i++) {
    tmp_disk[i] = origin_raid_disk[i];
    if (tmp_disk[i]->dirty) sum_d++;
  }
  int now_num = 0;
  int damagedDisk = 0;
  for (;tmp_disk[0] != NULL;) {
    cprintf("---------Check is going on---------------------Sector %d %d\n", now_num, sum_d);
    cprintf("start!\n");
    for (i = 0; i < 4; i++) {
      if (i == 3) cprintf("Parity value is: ");
      cprintf("%d ", tmp_disk[i]->data);
    }
    cprintf("\n");
    cprintf("end!\n");
    if (sum_d > 1) {
      cprintf("Sector %d cannot repair\n", now_num);
    } else if (sum_d == 1) {
      cprintf("Sector %d repair\n", now_num);
      cprintf("Wrong value is %d\n", tmp_disk[damagedDisk]->data);
      int rightValue = 0;
      for (j = 0; j < 4; ++j)
      {
        if (j != damagedDisk)
        {
          rightValue ^= tmp_disk[j]->data;
        }
      }
      tmp_disk[damagedDisk]->data = rightValue;
      cprintf("After fixing, the value is: ", rightValue);
      // tmp_disk[st]->data ;
      for (i = 0; i < 3; i++) {
        // if (i == 3) cprintf("Parity value is: ");
        cprintf("%d ", tmp_disk[i]->data);
      }
      cprintf("\n");
    }
    // }
    // cprintf("data\n");
    // for (i = 0; i < tmp_disk[2]->now; i++) {  
    //   t |= (!!(tmp_disk[2]->data & (1 << i))) <<  l;
    //   l++;
    //   t |= (!!(tmp_disk[4]->data & (1 << i))) <<  l;
    //   l++;
    //   t |= (!!(tmp_disk[5]->data & (1 << i))) <<  l;
    //   l++;
    //   t |= (!!(tmp_disk[6]->data & (1 << i))) <<  l;
    //   l++;
    //   if (l == 32) {
    //     cprintf("%d ", t);
    //     l = 0;
    //     t = 0;
    //   }
    // }
    // if (l != 0) {
    //   cprintf("%d", t);
    // }
    // cprintf("\n");
//    check_raid_disk();
    sum_d = 0;
    for (i = 0; i < 4; i++) {
      if (tmp_disk[i]->next == NULL) {
        cprintf("\n");
        cprintf("After Recovery\n");
        int count = 0;
        struct My_Disk* cur_disk;
        char c;
        while (raid_disks[count].next != NULL) {
          cur_disk = &raid_disks[count];
          c = cur_disk->data;
          if (count == 0 || (count - 3) % 4 != 0)
          {
            cprintf("%c", c);
          }
          count++;
        }
        return;
      }
      tmp_disk[i] = tmp_disk[i]->next;
      if (tmp_disk[i]->dirty) {
        sum_d++;
        damagedDisk = i;
      }
    }
    now_num++;
  }

  return;

}

// Dispatches to the correct kernel function, passing the arguments.
int32_t
syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4, uint32_t a5)
{
  // Call the function corresponding to the 'syscallno' parameter.
  // Return any appropriate return value.
  // LAB 3: Your code here.

    // panic("syscall not implemented");

  switch (syscallno) {
    case SYS_cputs:
      sys_cputs((char *)a1, a2);
      return 0;
    case SYS_vcprintf:
      return sys_vcprintf((const char*)a1, (va_list)a2);
    case SYS_cgetc:
      return sys_cgetc();
      //return 0;
    case SYS_getenvid:
      return sys_getenvid();
      //return 0;
    case SYS_env_destroy: 
      sys_env_destroy(a1);
      return 0;
    case SYS_yield:
      sys_yield();
      return 0;
    case SYS_exofork:
      return sys_exofork();
    case SYS_env_set_status:
      return sys_env_set_status(a1, a2);
    case SYS_page_alloc:
      return sys_page_alloc(a1, (void *)a2, a3);
    case SYS_page_map:
      return sys_page_map(a1, (void *)a2, a3, (void *)a4, a5);
    case SYS_page_unmap:
      return sys_page_unmap(a1, (void *)a2);
    case SYS_env_set_pgfault_upcall:
      return sys_env_set_pgfault_upcall(a1, (void *)a2);
    case SYS_ipc_recv:
      return sys_ipc_recv((void *)a1);
    case SYS_ipc_try_send:
      return sys_ipc_try_send(a1, a2, (void *)a3, a4);
    case SYS_env_set_gpfault_upcall:
      return sys_env_set_gpfault_upcall(((envid_t)a1), ((void *)a2));
    case SYS_env_set_divide0_upcall:
      return sys_env_set_divide0_upcall(((envid_t)a1), ((void *)a2));
    case SYS_env_set_illegalop_upcall:
      return sys_env_set_illegalop_upcall(((envid_t)a1), ((void *)a2));
    case SYS_env_set_trapframe:
      return sys_env_set_trapframe(a1, (struct Trapframe *)a2);
    case SYS_time_msec:
      return sys_time_msec();
    case SYS_net_pkt_transmit:
      return sys_net_pkt_transmit((void *) a1, (size_t) a2);
    case SYS_net_try_receive:
      return sys_net_try_receive((char *) a1, (int *) a2);
    case SYS_raid_init :
      sys_raid_init((uint32_t*) a1);
      goto succeed;
    case SYS_raid_add :
      sys_raid_add(a1, (uint32_t*) a2);
      goto succeed;
    case SYS_raid_change :
      sys_raid_change(a1, a2, a3);
      goto succeed;
    case SYS_raid_check :
      sys_raid_check();
      goto succeed;
    default:
      return -E_INVAL;
  }

  succeed : 
    return 0;
}

