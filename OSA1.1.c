/*
 ============================================================================
 Name        : OSA1.c
 Author      : William Idoine, wido998
 Version     : 1.0
 Description : Single thread implementation.
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#include "littleThread.h"
#include "threads1.c" // rename this for different threads

Thread newThread; // the thread currently being set up
Thread mainThread; // the main thread
struct sigaction setUpAction;
Thread currentThread;



/*
 * Switches execution from prevThread to nextThread.
 */
void switcher(Thread prevThread, Thread nextThread) {
	if (prevThread->state == FINISHED) { // it has finished
		printf("\ndisposing %d\n", prevThread->tid);
		free(prevThread->stackAddr); // Wow!
		//scheduler(prevThread, nextThread); //I added this
		longjmp(nextThread->environment, 1);
	} else if (setjmp(prevThread->environment) == 0) { // so we can come back here
		prevThread->state = READY;
		nextThread->state = RUNNING;
		currentThread = nextThread; //I added this
		printf("scheduling %d\n", nextThread->tid);
		longjmp(nextThread->environment, 1);
	}
}
//The general idea of scheduler is that, when a thread finishes, or when asked to go to another thread, or when made to go to another thread, you want to find the next thread in your list that is in the READY state. It is possible that the only thread still in the READY state is the same function, so make sure you account for that. If no threads are in the READY state, and all have FINISHED, then you will want to return to the mainThread.

void scheduler(Thread prevThread) {
	puts("\nSCHEDULER ONCE AGAIN");
	printf("Made it to scheduler and next threadID: %d \n",prevThread->next->tid);
	int threadAvailable = 0;
	switch(prevThread->next->state){
		case READY:
                	printf("WE HAVE FOUND A READY THREADID: %d", prevThread->next->tid);
			switcher(prevThread, prevThread->next);
			threadAvailable = 1;
                	break;
        	case FINISHED:
               		puts("Finished");
               		break;
        	case SETUP:
			puts("Setup");
                	break;
		case RUNNING:
			puts("Running");
                	break;		
		
	}
	threadAvailable = 0;
	puts("PAST THE SWITCH STATEMENT AND SHOULD GO BACK TO MAIN THREAD NOW");
	if(threadAvailable == 0){
		switcher(prevThread, mainThread);
	}

	
}


/*
 * Associates the signal stack with the newThread.
 * Also sets up the newThread to start running after it is long jumped to.
 * This is called when SIGUSR1 is received.
 */
void associateStack(int signum) {
	Thread localThread = newThread; // what if we don't use this local variable?
	localThread->state = READY; // now it has its stack
	if (setjmp(localThread->environment) != 0) { // will be zero if called directly
		(localThread->start)();
		puts("IN ASSOCIATE STACK");
		localThread->state = FINISHED;
		//currentThread = localThread;
		printf("THE NEXT THREAD FOR LOCAL IS: %d", localThread->next->tid);
		scheduler(localThread);
		//switcher(localThread, mainThread); // at the moment back to the main thread
	}
}

/*
 * Sets up the user signal handler so that when SIGUSR1 is received
 * it will use a separate stack. This stack is then associated with
 * the newThread when the signal handler associateStack is executed.
 */
void setUpStackTransfer() {
	setUpAction.sa_handler = (void *) associateStack;
	setUpAction.sa_flags = SA_ONSTACK;
	sigaction(SIGUSR1, &setUpAction, NULL);
}

/*
 *  Sets up the new thread.
 *  The startFunc is the function called when the thread starts running.
 *  It also allocates space for the thread's stack.
 *  This stack will be the stack used by the SIGUSR1 signal handler.
 */
Thread createThread(void (startFunc)()) {
	static int nextTID = 0;
	Thread thread;
	stack_t threadStack;

	if ((thread = malloc(sizeof(struct thread))) == NULL) {
		perror("allocating thread");
		exit(EXIT_FAILURE);
	}
	thread->tid = nextTID++;
	thread->state = SETUP;
	thread->start = startFunc;
	if ((threadStack.ss_sp = malloc(SIGSTKSZ)) == NULL) { // space for the stack
		perror("allocating stack");
		exit(EXIT_FAILURE);
	}
	thread->stackAddr = threadStack.ss_sp;
	threadStack.ss_size = SIGSTKSZ; // the size of the stack
	threadStack.ss_flags = 0;
	if (sigaltstack(&threadStack, NULL) < 0) { // signal handled on threadStack
		perror("sigaltstack");
		exit(EXIT_FAILURE);
	}
	newThread = thread; // So that the signal handler can find this thread
	kill(getpid(), SIGUSR1); // Send the signal. After this everything is set.
	return thread;
}
void printThreadStates(Thread threads[]){
		puts("\nThread States");
	     	puts("==============");
		for (int t = 0; t < NUMTHREADS; t++) {		
		printf("ThreadID: %d state: ", threads[t]->tid);
		switch (threads[t]->state) {
        		case READY:
                		puts("Ready");
                		break;
        		case FINISHED:
               			puts("Finished");
               			break;
        		case SETUP:
				puts("Setup");
                		break;
			case RUNNING:
				puts("Running");
                		break;
			}
		}
}

//void insertThread(Thread* head, Thread* new_thread)
//{
  // n is a pointer to the current node
 // Thread* n = &head;
  //while (NULL != (*n)) {
    // While the value pointed to by n is not NULL, we just keep
    // going to the next node in the list
  //  n = &(*n)->next;
 
 // }

  // Of course, once we hit a node that's NULL, we have hit the end of the
  // list, and so this is where we insert the new_node
 // *n = new_thread;
 //  n->prev =  
  
//}

int main(void) {
	struct thread controller;
	Thread threads[NUMTHREADS];
	mainThread = &controller;
	mainThread->state = RUNNING;
	setUpStackTransfer();
	for (int t = 0; t < NUMTHREADS; t++) {
		threads[t] = createThread(threadFuncs[t]);
	}
	//Thread* head = threads[0];
	for(int i = 0; i < NUMTHREADS; i++){
		if(i == 0){
		  	threads[i]->next = threads[i+1];
		   	threads[i]->prev = threads[NUMTHREADS-1];
		}
		else{
			threads[i]->next = threads[i+1];
			threads[i]->prev = threads[i-1];
		}
		//if(i==NUMTHREADS){
		//	threads[NUMTHREADS-1]->next = threads[0];
		//	threads[NUMTHREADS-1]->prev = threads[NUMTHREADS-2];
			//thread[2]->prev = threads[
		//}
		
	}
	threads[NUMTHREADS-1]->next = threads[0];
	threads[NUMTHREADS-1]->prev = threads[NUMTHREADS-2];
	//threads[NUMTHREADS]->next = threads[0];
	//threads[NUMTHREADS]->prev = threads[NUMTHREADS-1];
    	//node* n = thread[1];
    	//insertNode(head, n);
  	//Thread* tail = threads[1];
  	//insertThread(head, tail);
	printThreadStates(threads);
	puts("\nswitching to first thread\n");
	switcher(mainThread, threads[0]);
	puts("back to the main thread");
	printThreadStates(threads);
	return EXIT_SUCCESS;
}
