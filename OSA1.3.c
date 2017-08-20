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
#include <sys/time.h>

#include "littleThread.h"
#include "threads3.c" // rename this for different threads

Thread newThread; // the thread currently being set up
Thread mainThread; // the main thread
struct sigaction setUpAction;
struct itimerval timer;
struct sigaction sigact;
Thread currentThread;
Thread threadList[100];


/*
 * Switches execution from prevThread to nextThread.
 */
void switcher(Thread prevThread, Thread nextThread) {
	if (prevThread->state == FINISHED) { 
		printf("\ndisposing %d\n", prevThread->tid);
		free(prevThread->stackAddr); // Wow
		nextThread->state = RUNNING;  	
		currentThread = nextThread;    
		printThreadStates(threadList); 
		longjmp(nextThread->environment, 1); //this goes to associate stack
	} 
	else if (setjmp(prevThread->environment) == 0) { 
		prevThread->state = READY;
		nextThread->state = RUNNING;
		printThreadStates(threadList);  
		currentThread = nextThread;     
		longjmp(nextThread->environment, 1); //this also goes to associate stack
	}
	
}

void threadYield(){
	scheduler(currentThread);

}

void scheduler(Thread prevThread) {
		switch(prevThread->next->state){
			case READY:
				switcher(prevThread, prevThread->next);
		        	break;
			case RUNNING:
		        	break;	
			case FINISHED:

				if(prevThread->next == prevThread->prev){
					switcher(prevThread, mainThread);
					break;
				}
				else{
					scheduler(prevThread->next);
				}
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
		localThread->state = FINISHED;
		currentThread = localThread;
		localThread->next->prev = localThread->prev;
		localThread->prev->next = localThread->next;
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
		puts("\n");
}
void timer_handler(int signum){
	scheduler(currentThread);

}
//used http://www.informit.com/articles/article.aspx?p=23618&seqNum=14 to help make this code.
void setUpTimer(){
	 memset (&sigact, 0, sizeof (sigact));
	 sigact.sa_handler = &timer_handler;
	 sigaction (SIGVTALRM, &sigact, NULL);
	 timer.it_value.tv_sec = 0;
	 timer.it_value.tv_usec = 20000;
	 timer.it_interval.tv_sec = 0;
	 timer.it_interval.tv_usec = 20000;
	 setitimer (ITIMER_VIRTUAL, &timer, NULL);
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
	for(int i = 0; i < NUMTHREADS; i++){
		if(i == 0){
		  	threads[i]->next = threads[i+1];
		   	threads[i]->prev = threads[NUMTHREADS-1];
		}
		else{
			threads[i]->next = threads[i+1];
			threads[i]->prev = threads[i-1];
		}	
		threadList[i] =threads[i];	
	}
	threads[NUMTHREADS-1]->next = threads[0];
	threads[NUMTHREADS-1]->prev = threads[NUMTHREADS-2];
	setUpTimer();
	//need to call setUpTimer() here;
	printThreadStates(threadList);
	puts("\nswitching to first thread\n");
	switcher(mainThread, threads[0]);
	puts("\nback to the main thread");
	printThreadStates(threadList);
	return EXIT_SUCCESS;
}
