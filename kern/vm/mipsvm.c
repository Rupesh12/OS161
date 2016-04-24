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
#include <kern/errno.h>
#include <lib.h>
#include <spl.h>
#include <cpu.h>
#include <spinlock.h>
#include <proc.h>
#include <current.h>
#include <mips/tlb.h>
#include <addrspace.h>
#include <vm.h>

/*
 * Dumb MIPS-only "VM system" that is intended to only be just barely
 * enough to struggle off the ground. You should replace all of this
 * code while doing the VM assignment. In fact, starting in that
 * assignment, this file is not included in your kernel!
 *
 * NOTE: it's been found over the years that students often begin on
 * the VM assignment by copying dumbvm.c and trying to improve it.
 * This is not recommended. dumbvm is (more or less intentionally) not
 * a good design reference. The first recommendation would be: do not
 * look at dumbvm at all. The second recommendation would be: if you
 * do, be sure to review it from the perspective of comparing it to
 * what a VM system is supposed to do, and understanding what corners
 * it's cutting (there are many) and why, and more importantly, how.
 */

/* under dumbvm, always have 72k of user stack */
/* (this must be > 64K so argument blocks of size ARG_MAX will fit) */
#define DUMBVM_STACKPAGES    18

static bool vm_intialized = false ;
static unsigned int no_of_pages_in_coremap;

paddr_t firstaddr, lastaddr, freeaddr ;

struct coremap_entry* coremap ; 
/*
 * Wrap ram_stealmem in a spinlock.
 */
static struct spinlock stealmem_lock = SPINLOCK_INITIALIZER;

void
vm_bootstrap(void)
{
	// Changes made by Rupesh on 21 April 2016, following pearls in life

	spinlock_acquire(&stealmem_lock);
    unsigned int coremap_ends ;
    lastaddr=ram_getsize();
    firstaddr=ram_getfirstfree();
    spinlock_release(&stealmem_lock);
    
    no_of_pages_in_coremap = (lastaddr) / PAGE_SIZE ;
    kprintf("no of pages %d",no_of_pages_in_coremap);
    freeaddr = firstaddr + no_of_pages_in_coremap * sizeof(struct coremap_entry);
    freeaddr = ROUNDUP(freeaddr, PAGE_SIZE) ;
    
    coremap = (struct coremap_entry*)PADDR_TO_KVADDR(firstaddr);
    
    coremap_ends= (freeaddr)/PAGE_SIZE;
    for(unsigned int i=0 ; i< no_of_pages_in_coremap ; i++)
    {
        
        if(i<=coremap_ends)
        {
        coremap[i].page_state = 1 ; // Page is fixed
        coremap[i].physical_add = PAGE_SIZE * i;
        coremap[i].virtual_add = PADDR_TO_KVADDR(coremap[i].physical_add) ;
        coremap[i].page_begin = coremap[i].virtual_add ;
        }else
        {
        coremap[i].page_state = 0 ; // Page is avilable intially
        coremap[i].physical_add = PAGE_SIZE * i;
        coremap[i].virtual_add = PADDR_TO_KVADDR(coremap[i].physical_add) ;
        coremap[i].page_begin = coremap[i].virtual_add;
        //kprintf("%d",coremap[i].page_state);
        }
        
    }
	
    vm_intialized = true ;
    //no_of_pages_in_coremap=500;
}

/*
 * Check if we're in a context that can sleep. While most of the
 * operations in dumbvm don't in fact sleep, in a real VM system many
 * of them would. In those, assert that sleeping is ok. This helps
 * avoid the situation where syscall-layer code that works ok with
 * dumbvm starts blowing up during the VM assignment.
 */
 /*
static
void
dumbvm_can_sleep(void)
{
	if (CURCPU_EXISTS()) {
		// must not hold spinlocks 
		KASSERT(curcpu->c_spinlocks == 0);

		// must not be in an interrupt handler 
		KASSERT(curthread->t_in_interrupt == 0);
	}
}
*/
static
paddr_t
getppages(unsigned long npages)
{
	paddr_t addr;

	spinlock_acquire(&stealmem_lock);

	addr = ram_stealmem(npages);

	spinlock_release(&stealmem_lock);
	return addr;
}
/*
static
void
printcore(void)
{
	unsigned int i;
	for(i=0;i<no_of_pages_in_coremap;i++)
		kprintf("%d",coremap[i].page_state);
}
*/
/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(unsigned npages)
{	/*
	//dumb starts
	paddr_t pa;

	dumbvm_can_sleep();
	pa = getppages(npages);
	if (pa==0) {
		return 0;
	}
	return PADDR_TO_KVADDR(pa);
	//dumb ends
	*/
	//asst3 starts
	bool cons=true;
	unsigned int i=0,j;
	unsigned cnt;
	//spinlock_acquire(&stealmem_lock);
		//kprintf("llllllllllllllll%d",coremap[390].page_state);
	if(vm_intialized)
	{
		if(npages==1)
		{
			spinlock_acquire(&stealmem_lock);
			i=0;
			while(i<no_of_pages_in_coremap && coremap[i].page_state!=0)i++;
			if(i>=no_of_pages_in_coremap) 
			{
				spinlock_release(&stealmem_lock);
				return 0;
			}
				
			coremap[i].page_state=2;
			spinlock_release(&stealmem_lock);
			return coremap[i].virtual_add;
		}

		else 
		{
			spinlock_acquire(&stealmem_lock);
			i=0;
		m2:		while(i<no_of_pages_in_coremap && coremap[i].page_state!=0)i++;
				if(i>=no_of_pages_in_coremap)
				{ 
					spinlock_release(&stealmem_lock);
					return 0;
				}
				cons=true;
				j=i;
			for(cnt=0;cnt<npages;cnt++)
			{
				if(coremap[j].page_state!=0)
				{
					cons=false;
					i=j;
					break;
				}
				j++;
			}
			if(!cons)
			{
				goto m2;
			}
			else
			{	j=i;
				for(cnt=0;cnt<npages;cnt++)
				{
					coremap[j].page_state = 2 ; // Page is assigned
        			coremap[j].page_begin = coremap[i].virtual_add ;
        			j++;
				}
				
			spinlock_release(&stealmem_lock);
			}
			return coremap[i].virtual_add;
		}
	}
	else
	{ 
		return getppages(npages);
	}
	//asst3 ends
}

void
free_kpages(vaddr_t addr)
{
	/* nothing - leak the memory. */

	(void)addr;
	unsigned int i,j;
	spinlock_acquire(&stealmem_lock);
	i=0;
	while (i<no_of_pages_in_coremap && coremap[i].virtual_add!=addr)i++;
	if((coremap[i].virtual_add==addr && coremap[i].page_state!=1) && i<no_of_pages_in_coremap)
	{
		j=i;
		while(coremap[j].page_begin==coremap[i].virtual_add && j<no_of_pages_in_coremap)
		{
			coremap[j].page_state=0;
			coremap[j].page_begin=coremap[j].virtual_add;
			j++;
		}

	}

	spinlock_release(&stealmem_lock);
}

unsigned
int
coremap_used_bytes() {

	/* dumbvm doesn't track page allocations. Return 0 so that khu works. */

	 /* dumbvm doesn't track page allocations. Return 0 so that khu works. */
    unsigned int counter;
    unsigned int i  ;
    unsigned int result ;
    spinlock_acquire(&stealmem_lock);
    counter=0;
    for(i = 0 ; i<no_of_pages_in_coremap ; i++ )
    {
        if(coremap[i].page_state == 1 || coremap[i].page_state == 2){
            counter++ ;
        }
    }   

    result = counter * PAGE_SIZE ;
    spinlock_release(&stealmem_lock);
    return result;
}

void
vm_tlbshootdown_all(void)
{
	panic("dumbvm tried to do tlb shootdown?!\n");
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
	(void)ts;
	panic("dumbvm tried to do tlb shootdown?!\n");
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
	(void)faulttype;
	(void)faultaddress;
	return EFAULT;
}

