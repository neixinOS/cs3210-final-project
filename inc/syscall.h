#ifndef JOS_INC_SYSCALL_H
#define JOS_INC_SYSCALL_H

/* system call numbers */
enum {
  SYS_cputs = 0,
  SYS_vcprintf,
  SYS_cgetc,
  SYS_getenvid,
  SYS_env_destroy,
  SYS_page_alloc,
  SYS_page_map,
  SYS_page_unmap,
  SYS_exofork,
  SYS_env_set_status,
  SYS_env_set_trapframe,
  SYS_env_set_pgfault_upcall,
  SYS_yield,
  SYS_ipc_try_send,
  SYS_ipc_recv,

  SYS_time_msec,
  SYS_net_pkt_transmit,
  SYS_net_try_receive,

  SYS_env_set_gpfault_upcall,
  SYS_env_set_divide0_upcall,
  SYS_env_set_illegalop_upcall,

  SYS_raid_init,
  SYS_raid_add,
  SYS_raid_change,
  SYS_raid_check,

  NSYSCALLS
};

#endif  /* !JOS_INC_SYSCALL_H */
