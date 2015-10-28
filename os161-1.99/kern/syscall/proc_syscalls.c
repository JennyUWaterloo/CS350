#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
#include <mips/trapframe.h>
#include "opt-A2.h"
#include <synch.h>

extern struct array *procStructArray;

void sys__exit(int exitcode) {
  struct addrspace *as;
  struct proc *p = curproc;
  
  #if OPT_A2

    int parentLocation = locatePid(p->p_pid);
    struct procStruct *parentProcStr = array_get(procStructArray, parentLocation)
    parentProcStr->exitcode = _MKWAIT_EXIT(exitcode);
    cleanChildren(parentLocation);

    V(parentProcStr->proc_sem);

  #endif //OPT_A2

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}


int
sys_getpid(pid_t *retval)
{
  *retval = curproc->p_pid;
  return(0);
}

int
sys_waitpid(pid_t pid,
      userptr_t status,
      int options,
      pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }

  #if OPT_A2
  
  int location = locatePid(pid);
  struct procStruct *procStr = array_get(procStructArray, location);

  if (procStr->parent_pid != curproc->p_pid) {
    return (ECHILD);
  } else if (procStr == NULL) {
    return (ESRCH);
  }
  exitstatus = getExitCode(pid);

  #endif //OPT_A2

  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

int sys_fork(struct trapframe *tf, pid_t *retval) {
  int err;
  struct proc *fork_proc = proc_create_runprogram("fork_proc");

  if (fork_proc == NULL) {
    return ENOMEM;
  }
  else if (fork_proc == (struct proc *) ENPROC) {
    return ENPROC;
  }

  err = as_copy(curproc->p_addrspace, &fork_proc->p_addrspace);
  if (err) {
    proc_destroy(fork_proc);
    return err;
  }

  struct trapframe *fork_tf = kmalloc(sizeof(struct trapframe));
  if (fork_tf == NULL) {
    return ENOMEM;
  }

  memcpy(fork_tf, tf, sizeof(struct trapframe));

  err = thread_fork("fork_thread", fork_proc, (void* )enter_forked_process, fork_tf, 0);
  if (err) {
    proc_destroy(fork_proc);
    return -1;
  }

  *retval = fork_proc->p_pid;
  return 0;
}
