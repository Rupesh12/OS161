/*
 * Copyright (c) 2001, 2002, 2009
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

/*
 * Driver code is in kern/tests/synchprobs.c We will replace that file. This
 * file is yours to modify as you see fit.
 *
 * You should implement your solution to the stoplight problem below. The
 * quadrant and direction mappings for reference: (although the problem is, of
 * course, stable under rotation)
 *
 *   |0 |
 * -     --
 *    01  1
 * 3  32
 * --    --
 *   | 2|
 *
 * As way to think about it, assuming cars drive on the right: a car entering
 * the intersection from direction X will enter intersection quadrant X first.
 * The semantics of the problem are that once a car enters any quadrant it has
 * to be somewhere in the intersection until it call leaveIntersection(),
 * which it should call while in the final quadrant.
 *
 * As an example, let's say a car approaches the intersection and needs to
 * pass through quadrants 0, 3 and 2. Once you call inQuadrant(0), the car is
 * considered in quadrant 0 until you call inQuadrant(3). After you call
 * inQuadrant(2), the car is considered in quadrant 2 until you call
 * leaveIntersection().
 *
 * You will probably want to write some helper functions to assist with the
 * mappings. Modular arithmetic can help, e.g. a car passing straight through
 * the intersection entering from direction X will leave to direction (X + 2)
 * % 4 and pass through quadrants X and (X + 3) % 4.  Boo-yah.
 *
 * Your solutions below should call the inQuadrant() and leaveIntersection()
 * functions in synchprobs.c to record their progress.
 */

#include <types.h>
#include <lib.h>
#include <thread.h>
#include <test.h>
#include <synch.h>

/*
 * Called by the driver during initialization.
 */
//struct lock *quadrant0 , *quadrant1 , *quadrant2 , *quadrant3 ;
struct semaphore *quadrant0 , *quadrant1 , *quadrant2 , *quadrant3 ;
struct lock *control ;
void
stoplight_init() {
 // just delete every thing and free up the memory
 quadrant0 = sem_create("quadrant0",1) ; 
 quadrant1 = sem_create("quadrant1",1) ;  
 quadrant2 = sem_create("quadrant2",1) ;  
 quadrant3 = sem_create("quadrant3",1) ; 
 control =  lock_create("control") ;

return ;
}

/*
 * Called by the driver during teardown.
 */

void stoplight_cleanup() {
	sem_destroy(quadrant0);
	sem_destroy(quadrant1);
	sem_destroy(quadrant2);
	sem_destroy(quadrant3);
	
	
	return;

}

static
struct semaphore *
getQuadrant(int direction){
	if(direction == 0){
		return quadrant0 ;
	}
	else if(direction == 1){
		return quadrant1 ;
	}
	else if(direction == 2){
		return quadrant2 ;
	}
	else {
		return quadrant3 ;
	}
}

// void
// turnright(uint32_t direction, uint32_t index)
// {
// 	//(void)direction;
// 	//(void)index;
// struct lock *entry_lk ;

// 	int entry_dir ;

// 	entry_dir = direction ;

// 	//do
// 	//{
// 		//lock_acquire(control);
// 		    entry_lk = getQuadrant(entry_dir) ;
// 			lock_acquire(entry_lk);
// 			inQuadrant(entry_dir,index);
// 			lock_release(entry_lk);
// 			leaveIntersection(index);
							
// 		// 	if(!lock_do_i_hold(entry_lk)){
// 		// 	lock_release(entry_lk);
// 		// //	lock_release(control) ;
// 		// }
// 	//}while(!lock_do_i_hold(entry_lk))  ; 

	
	
	
	
	
// 	//lock_release(control) ;

// 	/*
// 	 * Implement this function.
// 	 */
// 	return;
// }

 void
 turnright(uint32_t direction, uint32_t index)
{
//	(void)direction;
//	(void)index;

	int entry_dir ; 
	entry_dir = direction ;
	struct semaphore *entry_sem ;
	int can_i_proced = 1 ; // only 0 means you can proced
	entry_sem = getQuadrant(entry_dir);


	do{
		lock_acquire(control) ;
		if(entry_sem->sem_count == 1)
		{
			P(entry_sem);
			
			can_i_proced = 0 ; // Yipee I can proceed now
			
		}
		lock_release(control);
	}
	while(can_i_proced != 0) ;

	while(can_i_proced == 0)
	{
		can_i_proced = 1 ;
		inQuadrant(entry_dir,index);
		leaveIntersection(index);
		V(entry_sem);

	}





	return ;
}
void
gostraight(uint32_t direction, uint32_t index)
{
	int entry_dir ; 
	entry_dir = direction ;
	int exit_dir ; 
	exit_dir = (direction+3) % 4 ;
	struct semaphore *entry_sem, *exit_sem ;
	int can_i_proced = 1 ; // only 0 means you can proced
	entry_sem = getQuadrant(entry_dir);
	exit_sem = getQuadrant(exit_dir);

	do{
		lock_acquire(control) ;
		if(entry_sem->sem_count == 1 && exit_sem->sem_count == 1)
		{
			P(entry_sem);
			P(exit_sem) ;
			can_i_proced = 0 ; // Yipee I can proceed now
			
		}
		lock_release(control);
	}
	while(can_i_proced != 0) ;

	while(can_i_proced == 0)
	{
		can_i_proced = 1 ;
		inQuadrant(entry_dir,index);
		inQuadrant(exit_dir,index) ;
		V(entry_sem);

		leaveIntersection(index);
		V(exit_sem);
	}




	return ;
}
// void
// gostraight(uint32_t direction, uint32_t index)
// {
// 	//(void)direction;
// 	//(void)index;
	
// 	struct lock *entry_lk , *exit_lk ;// *block_lk ;

// 	int entry_dir, exit_dir; // block_dir ;

// 	entry_dir = direction ;

// 	exit_dir = (direction+3)%4 ;

// 	//block_dir = (direction+2)%4 ;

	

// 	//do
// 	//{
// 		//lock_acquire(control);
// 		entry_lk = getQuadrant(entry_dir) ;
// 		lock_acquire(entry_lk);
// 		inQuadrant(entry_dir,index);
// 		exit_lk = getQuadrant(exit_dir) ;
			
// 		lock_acquire(exit_lk);
// 		inQuadrant(exit_dir,index) ;
// 		lock_release(entry_lk);
		
// 		lock_release(exit_lk);	
// 		leaveIntersection(index);
// //		block_lk = getQuadr                      ant(block_dir);
// //		lock_acquire(block_lk);
// 		//if(!lock_do_i_hold(entry_lk)||!lock_do_i_hold(exit_lk)||!lock_do_i_hold(block_lk)){
// 		// if(!lock_do_i_hold(entry_lk)||!lock_do_i_hold(exit_lk))
// 		// {
// 		// 	lock_release(entry_lk);
// 		// 	lock_release(exit_lk);
// 		// //	lock_release(block_lk);
// 		// //	lock_release(control) ;
// 		// }
// 	//}while(!lock_do_i_hold(entry_lk)&&!lock_do_i_hold(exit_lk))  ; 

	
	
	
	
	
// 	//lock_release(block_lk);
	

// 	//lock_release(control) ;




// 	/*
// 	 * Implement this function.
// 	 */
// 	return;
// }
// void
// turnleft(uint32_t direction, uint32_t index)
// {
// 	(void)direction;
// 	(void)index;
// 	/*
// 	 * Implement this function.
// 	 */

// 	  struct lock *entry_lk , *middle_lk, *last_lk ;

// 	 int entry_dir, middle_dir, last_dir ;

// 	 entry_dir = direction ;

// 	 middle_dir = (direction+3)%4 ;

// 	 last_dir = (direction+2)%4 ;

	

// 	 //do
// 	 //{
// 	 	//lock_acquire(control);
// 	 	entry_lk = getQuadrant(entry_dir) ;
// 	 	lock_acquire(entry_lk);
// 	 inQuadrant(entry_dir,index);
	 	
// 	 	middle_lk = getQuadrant(middle_dir) ;
	 

// 	 	lock_acquire(middle_lk);
// 	 	inQuadrant(middle_dir,index) ;
// 	 	lock_release(entry_lk);

// 	 	last_lk = getQuadrant(last_dir);
	 	
// 	 	lock_acquire(last_lk);
// 	 	inQuadrant(last_dir,index) ;
// 	 	lock_release(middle_lk);
// 	 	lock_release(last_lk);
// 	 	leaveIntersection(index);
	 	
	 
	 		
	 	
// 	 	// if(!lock_do_i_hold(entry_lk)||!lock_do_i_hold(middle_lk)||!lock_do_i_hold(last_lk)){
// 	 	// 	
// 	 	// 	lock_release(middle_lk);
// 	 	// 	lock_release(last_lk);
// 	 	// //	lock_release(control) ;
// 	 	// }
// 	 //}while(!lock_do_i_hold(entry_lk)&&!lock_do_i_hold(middle_lk)&&!lock_do_i_hold(last_lk))  ; 

	 
	
	 
	 
	 
	 
	
	
// 	 //lock_release(control) ;




// 	 /*
// 	 * Implement this function.
// 	*/ 
// 	return;
// }
void
turnleft(uint32_t direction, uint32_t index)
{
	//(void)direction;
	//(void)index;
	/*
	 * Implement this function.
	 */

	  

	 /*
	 * Implement this function.
	*/

	int entry_dir ; 
	entry_dir = direction ;
	int middle_dir ;
	middle_dir = (direction+3)%4 ;

	int exit_dir ; 
	exit_dir = (direction+2) % 4 ;
	
	struct semaphore *entry_sem, *middle_sem, *exit_sem ;
	int can_i_proced = 1 ; // only 0 means you can proced
	entry_sem = getQuadrant(entry_dir);
	middle_sem = getQuadrant(middle_dir); 
	exit_sem = getQuadrant(exit_dir);

	do{
		lock_acquire(control) ;
		if(entry_sem->sem_count == 1 && middle_sem->sem_count == 1&& exit_sem->sem_count == 1)
		{
			P(entry_sem);
			P(middle_sem);
			P(exit_sem) ;
			can_i_proced = 0 ; // Yipee I can proceed now
			
		}
		lock_release(control);
	}
	while(can_i_proced != 0) ;

	while(can_i_proced == 0)
	{
		can_i_proced = 1 ;
		inQuadrant(entry_dir,index);
		inQuadrant(middle_dir,index);
		V(entry_sem);
		inQuadrant(exit_dir,index);
		V(middle_sem) ;
		leaveIntersection(index);
		V(exit_sem);
	}




	
	return;
}