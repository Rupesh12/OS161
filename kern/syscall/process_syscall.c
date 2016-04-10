/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <syscall.h>
#include <kern/errno.h>
#include <lib.h>
#include <array.h>
#include <cpu.h>
#include <spl.h>
#include <uio.h> 
#include <spinlock.h>
#include <wchan.h>
#include <thread.h>
#include <threadlist.h>
#include <threadprivate.h>
#include <proc.h>
#include <current.h>
#include <synch.h>
#include <addrspace.h>
#include <mainbus.h>
#include <vnode.h>
#include <vfs.h>
#include <kern/fcntl.h> 
#include <mips/trapframe.h> 
#include <kern/wait.h> 
#include <proc.h>

/*
 * Example system call: get the time of day.
 */

int sys___fork(struct trapframe *p_tf,int *retval)
{
	//struct trapframe *tf=(struct trapframe *)kmalloc(sizeof(struct trapframe));
	if(p_tf==NULL)
		return ENOMEM;
	struct trapframe* tf = (struct trapframe*)kmalloc(sizeof(struct trapframe));
	if(tf==NULL)
		return ENOMEM;
	*tf=*p_tf;
	//struct addrspace *as;
	int result;
	//result=as_copy(curthread->t_proc->p_addrspace, &as);
	//if(result)
	//	return ENOMEM;
	struct proc *child=proc_create_runprogram("child");
	if(child==NULL)
		return ENOMEM;
	result=as_copy(curthread->t_proc->p_addrspace, &child->p_addrspace);
	if(result)
		return ENOMEM;
	//thread fork function here
	//result = thread_fork(curthread->t_name,child,child_forkentry,(void *) tf,(unsigned long) as);
	result=thread_fork(curthread->t_name,child,child_forkentry,(void *) tf,(unsigned long) child->p_addrspace);
	if(result)
	{
		//kfree(tf);
		//tf=NULL;
		//proc_destroy(child);
		return ENOMEM;
	}
		

	/*
	for(int i=0;i<OPEN_MAX;i++)
	{
		child->file_table[i]=curthread->file_table[i];
		if(child->file_table[i]!=NULL)
			child->file_table[i]->reference++;
	}
	
	process_table[child->process_id]->ppid=curthread->process_id;
	*/
	//kprintf("returned value is %d",child_id);
	*retval=child->process_id;
	return 0;
	

} 

void child_forkentry(void *data1, unsigned long data2)
{
	struct trapframe *tf=(struct trapframe *)data1;
	struct trapframe tf2 = *tf;
	tf2.tf_a3 = 0;
	tf2.tf_v0 = 0;
	tf2.tf_epc += 4;
	struct addrspace *as = (struct addrspace*)data2;
	as_copy(as, &curthread->t_proc->p_addrspace);
	as_activate();
	mips_usermode(&tf2); 

}

int sys___getpid(int *retval)
{
	*retval=curthread->process_id;
	return 0;
}



int
sys_waitpid(pid_t pid, int *status, int options,int *retval)
{
	if(process_table[pid]->exited==false)
		P(process_table[pid]->exitsem);
	int result;

	result = copyout((const void *) &(process_table[pid]->exitcode), (userptr_t) status,
			sizeof(int));

	int a=options;
	a=result;
	a++;

	*retval=pid;
	return 0;
}

int 
sys_exit(int code)
{
		process_table[curthread->process_id]->exitcode=_MKWAIT_EXIT(code);
		process_table[curthread->process_id]->exited=true;
		V(process_table[curthread->process_id]->exitsem);
		thread_exit(); 
		return 0;
}

int
execv(char *program, char **args)
{
	(void)program;
	(void)args;
	struct addrspace *as;
	struct vnode *v;
	vaddr_t entrypoint, stackptr;
	int result;


	/* Open the file. */
	result = vfs_open(program, O_RDONLY, 0, &v);
	if (result) {
		return result;
	}
	as_destroy(proc_getas());
	proc_setas(NULL);
	/* We should be a new process. */
	KASSERT(proc_getas() == NULL);

	/* Create a new address space. */
	as = as_create();
	if (as == NULL) {
		vfs_close(v);
		return ENOMEM;
	}

	/* Switch to it and activate it. */
	proc_setas(as);
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

	/* Warp to user mode. */
	enter_new_process(0 /*argc*/, NULL /*userspace addr of argv*/,
			  NULL /*userspace addr of environment*/,
			  stackptr, entrypoint);

	/* enter_new_process does not return. */
	panic("enter_new_process returned\n");
	return EINVAL;
}