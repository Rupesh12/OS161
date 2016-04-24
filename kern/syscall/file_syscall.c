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
#include <kern/seek.h> 
#include <kern/stat.h> 


/*
 * Example system call: get the time of day.
 */

void initialize_ft(void)
{
	int i;
	char filepath[]="con:";
	struct vnode *node;
	curthread->file_count=3;

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
	int kflags;

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
		

	error=copyin((const_userptr_t) flags, &kflags, sizeof(int));	
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
	    curthread->file_table[i]->mode=kflags; // to have a check on permissions
	    curthread->file_table[i]->reference=1 ; 

	
	*retval=i;

	return 0;
}
/*
int
sys_close(int fd)
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
*/
int
sys___close(int fd) //after TA's corrections
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
		curthread->file_table[fd]->reference--;
		lock_release(curthread->file_table[fd]->lock);
	   lock_destroy(curthread->file_table[fd]->lock);
	    entry=curthread->file_table[fd];
		curthread->file_table[fd]=NULL;
		curthread->file_count--;
		kfree(entry);

	}

	else
		{curthread->file_table[fd]->reference--;

	// Buggy code, needed to be rectified done // :)

		lock_release(curthread->file_table[fd]->lock);
	}
	// lock_destroy(curthread->file_table[fd]->lock);

	// entry=curthread->file_table[fd];
	// curthread->file_table[fd]=NULL;
	// curthread->file_count--;
	// kfree(entry);

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

	if(file==NULL)
		return EBADF;

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

	if(file==NULL || file->mode==O_RDONLY)
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

off_t
sys_lseek(int file_handle, off_t pos, int whence,int *retval,int *retval1)
{
    
    int error ;
    off_t new_pos ; 
    struct stat temp ;
    // Checking that passed arguments are correct
    if(file_handle<0 || file_handle>= OPEN_MAX || curthread->file_table[file_handle]== NULL )
    {
        *retval = -1 ;
        return EBADF ;
    }

    lock_acquire(curthread->file_table[file_handle]->lock);

    switch(whence){
        case SEEK_SET: 
            new_pos = pos ;
            break ;

        case SEEK_CUR:
            new_pos = curthread->file_table[file_handle]->offset +pos ;
            break ;

        case SEEK_END:
            error = VOP_STAT(curthread->file_table[file_handle]->vn, &temp) ;
            if(error)
            {
                *retval = -1 ;
                return EINVAL ;
            }
            new_pos = temp.st_size + pos ;
            break;
            default :
                lock_release(curthread->file_table[file_handle]->lock);
                return EINVAL ;
    }

    if(new_pos < 0)
    {
        lock_release(curthread->file_table[file_handle]->lock);
        return EINVAL ;
    }
    // pOTENTIAL error of arguments
    error = VOP_ISSEEKABLE(curthread->file_table[file_handle]->vn) ;
    // Modifaction for Is seekable
    if(!error)
    {
        lock_release(curthread->file_table[file_handle]->lock);
        return error ;
    }

    // All check done 
    // Updating the offset in the file_table
    curthread->file_table[file_handle]->offset  = new_pos ;

    *retval = (uint32_t)((new_pos & 0xFFFFFFFF00000000) >> 32);
    *retval1 = (uint32_t)(new_pos & 0xFFFFFFFF);

    lock_release(curthread->file_table[file_handle]->lock);
    return 0 ;
}   

int
dup2(int oldfd, int newfd, int *retval){

	int error ;

	// checking the the arguments passed points to a valid file handle to be cloned.
	if(oldfd >= OPEN_MAX || oldfd< 0 || newfd >= OPEN_MAX || newfd < 0)
	{
		*retval = -1 ;
		return EBADF ;
	}

	// if both the oldfd and newfd are equal, then just set the newfd = old fd

	if(oldfd == newfd){
		*retval = newfd ;
		return 0 ;
	}

	// if the new file handle is already opend, then close that file first before clonning
	if(curthread->file_table[newfd] != NULL){
		error = sys___close(newfd) ;

			// if there is an error inclosing that file return it
		if(error){
			*retval = -1 ;
			return error ;
		}
	}
	if(curthread->file_table[oldfd]== NULL)
	{
		*retval = -1 ;
		return error ;
	}

	// If every is awesome lol ... then let's start clonning :)
	lock_acquire(curthread->file_table[oldfd]->lock) ;
	
	curthread->file_table[oldfd]->reference++; // incrementing the reference counter to synchronize the closing operation
  
    curthread->file_table[newfd]->name = kstrdup(curthread->file_table[oldfd]->name);
	curthread->file_table[newfd]-> vn = curthread->file_table[oldfd]-> vn ;
	curthread->file_table[newfd]-> lock = lock_create("dup2 lock");
	curthread->file_table[newfd]-> offset = curthread->file_table[oldfd]->offset ;
	curthread->file_table[newfd]->mode = curthread->file_table[oldfd]->mode ;
	curthread->file_table[newfd]->reference = curthread	->file_table[oldfd]->reference ;
	//End of Clonning :) :)
		



	 
	
	lock_release(curthread->file_table[oldfd]->lock);
	
	*retval = newfd ;
	return 0 ; 
}
