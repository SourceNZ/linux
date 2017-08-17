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
Thread threadList[100];


/*
 * Switches execution from prevThread to nextThread.
 */
void switcher(Thread prevThread, Thread nextThread) {
	if (prevThread->state == FINISHED) { // it has finished
		printf("\ndisposing %d\n", prevThread->tid);
		free(prevThread->stackAddr); // Wow
		longjmp(nextThread->environment, 1); //this goes to associate stack
	} else if (setjmp(prevThread->environment) == 0) { // this goes to
		puts("WANT TO BE HERE AFTER DISPOSING");
		prevThread->state = READY;
		nextThread->state = RUNNING;
		printThreadStates(threadList);
		currentThread = nextThread; //I added this
		printf("scheduling %d\n", nextThread->tid);
		longjmp(nextThread->environment, 1); //this also goes to associate stack
	}
}


void scheduler(Thread prevThread) {
	puts("\nSCHEDULER SCHEDULER SCHEDULER");
	int threadAvailable = 0;
	switch(prevThread->next->state){
		case READY:
			switcher(prevThread, prevThread->next);
			threadAvailable = 1;
                	break;
        	case FINISHED:
               		puts("ALL THREADS FINISHED");
			threadAvailable = 0;
               		break;
        	case SETUP:
			puts("Setup");
			threadAvailable = 0;
                	break;
		case RUNNING:
			switcher(prevThread, prevThread->next);
			puts("Running");
			threadAvailable = 1;
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
	Thread localThread = newThread; 
	localThread->state = READY; 
	if (setjmp(localThread->environment) != 0) { 
		(localThread->start)();
		puts("IN ASSOCIATE STACK");
		//printThreadStates(threadList);
		localThread->state = FINISHED;
		scheduler(localThread);
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


int main(void) {
	struct thread controller;
	Thread threads[NUMTHREADS];
	mainThread = &controller;
	mainThread->state = RUNNING;
	setUpStackTransfer();
	for (int t = 0; t < NUMTHREADS; t++) {
		threads[t] = createThread(threadFuncs[t]);
		threadList[t] =threads[t];
	}
	//threadList = threads;
	for(int i = 0; i < NUMTHREADS; i++){
		if(i == 0){
		  	threads[i]->next = threads[i+1];
		   	threads[i]->prev = threads[NUMTHREADS-1];
		}
		else{
			threads[i]->next = threads[i+1];
			threads[i]->prev = threads[i-1];
		}
	
		
	}
	threads[NUMTHREADS-1]->next = threads[0];
	threads[NUMTHREADS-1]->prev = threads[NUMTHREADS-2];


	printThreadStates(threadList);
	puts("\nswitching to first thread\n");
	printThreadStates(threadList);
	switcher(mainThread, threads[0]);
	puts("back to the main thread");
	printThreadStates(threadList);
	return EXIT_SUCCESS;
}
