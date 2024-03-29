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

#ifndef _SYSCALL_H_
#define _SYSCALL_H_
#define MAX_RUNNING_PROCS 256

#include <cdefs.h> /* for __DEAD */
struct trapframe; /* from <machine/trapframe.h> */
struct process *process_table[MAX_RUNNING_PROCS];
pid_t child_id;
char **execbuf;

/*
 * The system call dispatcher.
 */

void syscall(struct trapframe *tf);

/*
 * Support functions.
 */

/* Helper for fork(). You write this. */
void enter_forked_process(struct trapframe *tf);

/* Enter user mode. Does not return. */
__DEAD void enter_new_process(int argc, userptr_t argv, userptr_t env,
		       vaddr_t stackptr, vaddr_t entrypoint);


/*
 * Prototypes for IN-KERNEL entry points for system call implementations.
 */

int sys_reboot(int code);
int sys___time(userptr_t user_seconds, userptr_t user_nanoseconds);
void initialize_ft(void);
int sys___open(char *filename, int flags,mode_t mode,int *retval);
int sys___close(int fd);
ssize_t sys___read(int fd, void *buf, size_t buflen,int *retval);
ssize_t sys___write(int fd, void *buf, size_t buflen,int *retval);
void sys___exit(int exitcode);
int sys___fork(struct trapframe *p_tf,int *retval);
void child_forkentry(void *data1, unsigned long data2);
int sys___getpid(int *retval);
pid_t sys___waitpid(pid_t pid, int *status, int options,int *retval);
int sys_waitpid(pid_t pid, int *status, int options,int *retval);
int sys_exit(int exitcode);
int execv(char *program, char **args);
off_t sys_lseek(int file_handle, off_t pos, int whence,int *retval,int *retval1);
int execv_test(char *program, char **args);
int dup2(int oldfd, int newfd, int *retval);

#endif /* _SYSCALL_H_ */
