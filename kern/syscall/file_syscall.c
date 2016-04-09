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


/*
 * Example system call: get the time of day.
 */

void initialize_ft(void)
{
	int i;
	char filepath[]="con:";
	struct vnode *node;

	vfs_open(filepath, O_RDONLY, 0, &node);

		curthread->file_table[0]=kmalloc(sizeof(struct filehandle_structure));
		curthread->file_table[0]->name=filepath; //file name
	    curthread->file_table[0]->vn=node;
	    curthread->file_table[0]->lock=lock_create("file ") ; // lock for synchronization
	    curthread->file_table[0]->offset=0 ; // file offset
	    curthread->file_table[0]->mode=O_RDONLY; // to have a check on permissions
	    curthread->file_table[0]->reference=1 ; 

	for(i=1;i<3;i++)
	{
		vfs_open(filepath, O_WRONLY, 0, &node);
		curthread->file_table[i]=kmalloc(sizeof(struct filehandle_structure));
		curthread->file_table[i]->name=filepath; //file name
	    curthread->file_table[i]->vn=node;
	    curthread->file_table[i]->lock=lock_create("file ") ; // lock for synchronization
	    curthread->file_table[i]->offset=0 ; // file offset
	    curthread->file_table[i]->mode=O_WRONLY; // to have a check on permissions
	    curthread->file_table[i]->reference=1 ; 
	}    

	
}

int 
sys___open(char *filename, int flags,mode_t mode, int *retval)
{
	int i;
	char filepath[PATH_MAX];
	struct vnode *node;
	int error;
	size_t temp;

	if(filename==NULL ) 
		return EFAULT; //kuch likhana hai user space me rakhana hai 

	/*if(!(flags==0 || flags==1 || flags==2 || flags==3 || flags==4 || flags==8 || flags==16 || flags==32 || flags==64)) // handelling invalid flag values error
		return EINVAL;
	*/
		
	if(flags<0 || flags >64) // handelling invalid flag values error
		return EINVAL;
		

	//add
	//filepath=kmalloc(sizeof(char)*PATH_MAX);
	error= copyinstr((const_userptr_t) filename, filepath, PATH_MAX, &temp);

		if (error)
			return error;
		

	//end
	
	error = vfs_open(filepath, flags, mode, &node);
	
	if(error)
		return EINVAL;

	for(i=3;i<OPEN_MAX;i++)
	{
		if(curthread->file_table[i]==NULL)
			break;
	}

	if(i==OPEN_MAX)
		return EMFILE;

		curthread->file_count++;
		curthread->file_table[i]=kmalloc(sizeof(*curthread->file_table[i]));
		curthread->file_table[i]->name=filepath; //file name
	    curthread->file_table[i]->vn=node;
	    curthread->file_table[i]->lock=lock_create(filepath) ; // lock for synchronization
	    curthread->file_table[i]->offset=0 ; // file offset
	    curthread->file_table[i]->mode=flags; // to have a check on permissions
	    curthread->file_table[i]->reference=1 ; 

	
	*retval=i;

	return 0;
}

int
sys___close(int fd)
{
	file_handle *entry;

	if(fd<0 || fd>= OPEN_MAX)
		return EBADF;
	
	if (curthread->file_table[fd]==NULL)
		return EBADF;

	lock_acquire(curthread->file_table[fd]->lock);
	if(curthread->file_table[fd]->reference ==1)
	{
		vfs_close(curthread->file_table[fd]->vn);
	}

	else
		curthread->file_table[fd]->reference--;

	lock_release(curthread->file_table[fd]->lock);
	lock_destroy(curthread->file_table[fd]->lock);

	entry=curthread->file_table[fd];
	curthread->file_table[fd]=NULL;
	curthread->file_count--;
	kfree(entry);

	return 0;

}

ssize_t sys___read(int fd, void *buf, size_t buflen, int *retval)
{
	size_t stoplen;
	int ret;
	file_handle *file;
	struct vnode *node;
	struct uio u;
	struct iovec iov;

	if(fd<0 || fd>= OPEN_MAX || curthread->file_table[fd]==NULL)
		return EBADF;

	ret=copycheck2((const_userptr_t)buf, buflen, &stoplen);

	if(ret)
		return EFAULT;

	file=curthread->file_table[fd];

	if(file->mode==O_WRONLY)
		return EBADF;

	node=file->vn;

	lock_acquire(file->lock);
	iov.iov_ubase = (userptr_t)buf;
	iov.iov_len = buflen;		 // length of the memory space
	u.uio_iov = &iov;
	u.uio_iovcnt = 1;
	u.uio_resid = buflen;          // amount to read from the file
	u.uio_offset = file->offset;
	u.uio_segflg = UIO_USERSPACE;
	u.uio_rw = UIO_READ;
	u.uio_space = curthread->t_proc->p_addrspace;

	ret=VOP_READ(node,&u);

	if(ret)
	{
		lock_release(file->lock);
		return EFAULT;
	}

	file->offset=u.uio_offset;
	*retval = buflen-u.uio_resid;
	lock_release(file->lock);

	return 0;

}

ssize_t sys___write(int fd, void *buf, size_t buflen, int *retval)
{
	size_t stoplen;
	int ret;
	file_handle *file;
	struct vnode *node;
	struct uio u;
	struct iovec iov;

	if(fd<0 || fd>= OPEN_MAX || curthread->file_table[fd]==NULL)
		return EBADF;

	ret=copycheck2((const_userptr_t)buf, buflen, &stoplen);

	if(ret)
		return EFAULT;

	file=curthread->file_table[fd];

	if(file->mode==O_RDONLY)
		return EBADF;

	node=file->vn;

	lock_acquire(file->lock);
	uio_kinit(&iov,&u,buf,buflen,file->offset,UIO_WRITE);
	
	ret=VOP_WRITE(node,&u);

	if(ret)
	{
		lock_release(file->lock);
		return EFAULT;
	}

	file->offset=u.uio_offset;
	*retval=buflen-u.uio_resid;
	lock_release(file->lock);
	

	return 0;

}