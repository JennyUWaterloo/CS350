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

  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  
  #if OPT_A2



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
  *retval = curproc->pid;
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


  #endif OPT_A2

  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

int sys_fork(struct trapframe *tf, pid_t *retval) {
  int error;
  struct proc *fork_proc = proc_create_runprogram("fork_proc");

  if (fork_proc == NULL) {
    return(ENOMEM);
  }
  else if (fork_proc == (struct proc *) ENPROC) {
    return (ENPROC);
  }

  struct addrSpace *fork_addrSpace = kmalloc(sizeof(struct addrspace));

  error = as_copy(curproc->p_addrspace, &fork_addrSpace);
  if (error) {
    proc_destroy(fork_proc);
    return error;
  }

  fork_proc->p_addrspace = fork_addrSpace;

  temp_TF = kmalloc(sizeof(struct trapframe));
  if (temp_TF == NULL) {
    return ENOMEM;
  }

  memcpy(temp_TF, tf, sizeof(struct trapframe));

  error = thread_fork("fork_thread", fork_proc, enter_forked_process, NULL, 0);
  if (error) {
    proc_destroy(fork_proc);
    return -1;
  }

  *retval = fork_proc->pid;
  return 0;
}