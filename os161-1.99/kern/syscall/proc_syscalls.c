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
    struct procStruct *parentProcStr = array_get(procStructArray, parentLocation);
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

int sys_execv(const char *program, char **args) {
	int err;

	if (program == NULL) return ENOENT;

	struct addrspace *oldAddrspace = curproc_getas();

	int argsCount;
	while (args[argsCount] != NULL) {
		if (strlen(args[argsCount]) >= 1025) return E2BIG;
		argsCount++;
	}
	if (argsCount >= 65) {
		return E2BIG;
	}

	char **newArgs = kmalloc((argsCount+1) * sizeof(char));
	for (int i = 0; i < argsCount; i++) {
		newArgs[i] = kmalloc((strlen(args[i])+1) * sizeof(char));

		err = copyinstr((userptr_t)args[i], newArgs[i], strlen(args[i])+1, NULL);
		if (err) return err;
	}

	newArgs[argsCount] = NULL;

	char *newProgram = kmalloc(sizeof(char*)*(strlen(program)+1));
	if (newProgram == NULL) return ENOMEM;

	int result = copyinstr((userptr_t)program, newProgram, strlen(program)+1, NULL);
	if (result != 0) return ENOEXEC;

	//start of copy from kern/syscall/runprogram.c

	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;

	/* Open the file. */
	result = vfs_open(progname, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}

	/* Create a new address space. */
	as = as_create();
	if (as ==NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	curproc_setas(as);
	as_activate();

	/* Load the executable. */
	result = load_elf(v, &entrypoint);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		vfs_close(v);
		return result;
	}

	/* Done with the file now. */
	vfs_close(v);

	/* Define the user stack in the address space */
	result = as_define_stack(as, &stackptr);
	if (result) {
		/* p_addrspace will go away when curproc is destroyed */
		return result;
	}

	//end of copy

	//align - when storing items on the stack, pad each item such that they are 8-byte aligned
	while (stackptr % 8 != 0) {
		stackptr--;
	}

	vaddr_t argsptr;

	for (int i = argsCount-1; i >= 0; i--) {
		stackptr = stackptr - strlen(newArgs[i]) + 1;

		err = copyoutstr(newArgs[i], (userptr_t)stackptr, strlen(newArgs[i]) + 1, NULL);
		if (err) return err;

		argsptr[i] = stackptr;
	}

	//align again - Strings don't have to be 4 or 8-byte aligned. However, pointers to strings need to be 4-byte aligned
	while (stackptr % 4 != 0) {
		stackptr--;
	}

	argsptr[argsCount] = 0;

	for (int i = argsCount; i >= 0; i--) {
		stackptr = stackptr - ROUNDUP(sizeof(vaddr_t), 4);
		err = copyout(&argsptr[i], (userptr_t)stackptr, sizeof(vaddr_t));
		if (err) return err;
	}

	as_destroy(oldAddrspace);

	enter_new_process(argsCount, (userptr_t)stackptr, stackptr, entrypoint);
}
