/*
 * Copyright (c) 2001, 2002, 2009
 *	The President and Fellows of Harvard College.
 *?

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

/*
 * Driver code is in kern/tests/synchprobs.c We will
 * replace that file. This file is yours to modify as you see fit.
 *
 * You should implement your solution to the whalemating problem below.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */

	static volatile int m=0;
	static volatile int f=0;
	static volatile int mm=0;
	static volatile int all_set_f=1;
	static volatile int all_set_mm=0;
	static volatile int all_set_m=0;	

	struct cv *whm;
	struct cv *whf;
	struct cv *whmm;
	struct lock *whmlock;
	struct lock *whflock;
	struct lock *whmmlock;
	//struct spinlock *sl;
	
void whalemating_init() {
	whm=cv_create("whm");
	whf=cv_create("whf");
	whmm=cv_create("whmm");
	whmlock=lock_create("whmlock");
	whflock=lock_create("whflock");
	whmmlock=lock_create("whmmlock");
	//spinlock_init(sl);

	//(void)whm;
	return;
	
}



/*
 * Called by the driver during teardown.
 */


void
whalemating_cleanup() {
	cv_destroy(whm);
	cv_destroy(whf);
	cv_destroy(whmm);
	lock_destroy(whmlock);
	lock_destroy(whflock);
	lock_destroy(whmmlock);
	//spinlock_cleanup(sl);
	return;
}

void
male(uint32_t index)
{
	//(void)index;
	//spinlock_acquire(sl);
	lock_acquire(whmlock);
	m++;
	male_start(index);
	lock_release(whmlock);
	lock_acquire(whmlock);
	while((f==0 || mm==0) && all_set_f==0 && all_set_mm==0)
	{
		cv_wait(whm,whmlock);
	}
	
	all_set_f=1;
	all_set_mm=1;
	all_set_m=0;
	lock_release(whmlock);
	
	if(all_set_f==1){

	lock_acquire(whflock);
	cv_signal(whf,whflock);
	lock_release(whflock);}
	if(all_set_mm==1){
	lock_acquire(whmmlock);
	
	cv_signal(whmm,whmmlock);
	lock_release(whmmlock);}
	lock_acquire(whmlock);
	male_end(index);
	m--;
	all_set_m=0;
	lock_release(whmlock);
	
	
	//spinlock_release(sl);
		 
	return;
}

void
female(uint32_t index)
{
	//(void)index;
	//spinlock_acquire(sl);
	lock_acquire(whflock);
	f++;
	female_start(index);
	lock_release(whflock);
	lock_acquire(whflock);
	while((m==0 || mm==0)&& all_set_m==0 && all_set_mm==0)
	{
		cv_wait(whf,whflock);
	}
	
	all_set_f=0;
	all_set_mm=1;
	all_set_m=1;
	lock_release(whflock);
	
	if(all_set_m==1){
	lock_acquire(whmlock);
    cv_signal(whm,whmlock);
    lock_release(whmlock);}
    if(all_set_mm==1){
    lock_acquire(whmmlock);
    
    cv_signal(whmm,whmmlock);
    lock_release(whmmlock);}
	lock_acquire(whflock);
	female_end(index);
	f--;
	all_set_f=0;
	lock_release(whflock);
	//spinlock_release(sl);
	/*
	 * Implement this function by calling female_start and female_end when
	 * appropriate.
	 */
	return;
}

void
matchmaker(uint32_t index)
{
	//(void)index;
	//spinlock_acquire(sl);
	lock_acquire(whmmlock);
	mm++;
	matchmaker_start(index);
	lock_release(whmmlock);
	lock_acquire(whmmlock);
	while((m==0 || f==0)&& all_set_m==0&&all_set_f==0)
	{
		cv_wait(whmm,whmmlock);
	}
	
	lock_release(whmmlock);
	all_set_f=1;
	all_set_mm=0;
	all_set_m=1;
	if(all_set_m==1){
	lock_acquire(whmlock);
	
	cv_signal(whm,whmlock);
	lock_release(whmlock);}
	if(all_set_f==1){
	lock_acquire(whflock);
	
	cv_signal(whf,whflock);
	lock_release(whflock);}
	lock_acquire(whmmlock);
	matchmaker_end(index);
	mm--;
	all_set_mm=0;
	lock_release(whmmlock);
	//spinlock_release(sl);
	/*
	 * Implement this function by calling matchmaker_start and matchmaker_end
	 * when appropriate.
	 */
	return;
}
