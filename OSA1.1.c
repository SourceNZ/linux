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
#include "threads1.c" 

Thread newThread; // the thread currently being set up
Thread mainThread; // the main thread
struct sigaction setUpAction;
Thread currentThread;
Thread threadList[100];

void switcher(Thread prevThread, Thread nextThread) {
	if (prevThread->state == FINISHED) { 
		printf("\ndisposing %d\n", prevThread->tid);
		free(prevThread->stackAddr); // Wow
		nextThread->state = RUNNING;   //I added this
		currentThread = nextThread;    //I added this
		printThreadStates(threadList); //I added this
		longjmp(nextThread->environment, 1); //this goes to associate stack
	} else if (setjmp(prevThread->environment) == 0) { 
		prevThread->state = FINISHED;
		nextThread->state = RUNNING;
		printThreadStates(threadList);  //I added this
		currentThread = nextThread;     //I added this
		longjmp(nextThread->environment, 1); //this also goes to associate stack
	}
}

void scheduler(Thread prevThread) {
		switch(prevThread->next->state){
			case READY:
				switcher(prevThread, prevThread->next);
		        	break;
			case SETUP:
				prevThread->next->prev = prevThread->prev;
				prevThread->prev->next = prevThread->next;
				if(prevThread->next == prevThread->prev){
					switcher(prevThread, mainThread);
					break;
				}
				else{
					scheduler(prevThread->next);
		       			break;
				}
		        	break;
			case RUNNING:
		        	break;	
			case FINISHED:
				prevThread->next->prev = prevThread->prev;
				prevThread->prev->next = prevThread->next;
				if(prevThread->next == prevThread->prev){
					prevThread->state = READY;
					//free(prevThread->stackAddr);
					switcher(prevThread, mainThread);
					break;
				}
				else{
					scheduler(prevThread->next);
		       			break;
				}
		}
}

void associateStack(int signum) {
	Thread localThread = newThread; 
	localThread->state = READY; 
	if (setjmp(localThread->environment) != 0) { 
		(localThread->start)();
		localThread->state = FINISHED;
		currentThread = localThread;
		scheduler(localThread);

	}
}


void setUpStackTransfer() {
	setUpAction.sa_handler = (void *) associateStack;
	setUpAction.sa_flags = SA_ONSTACK;
	sigaction(SIGUSR1, &setUpAction, NULL);
}

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

int main(void) {
	struct thread controller;
	Thread threads[NUMTHREADS];
	mainThread = &controller;
	mainThread->state = RUNNING;
	setUpStackTransfer();
	for (int t = 0; t < NUMTHREADS; t++) {
		threads[t] = createThread(threadFuncs[t]);
		threadList[t] = threads[t];
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
	}
	threads[NUMTHREADS-1]->next = threads[0];
	threads[NUMTHREADS-1]->prev = threads[NUMTHREADS-2];
	printThreadStates(threadList);
	puts("\nswitching to first thread\n");
	printThreadStates(threadList);
	//threads[2]->state = FINISHED;
	switcher(mainThread, threads[0]);
	puts("\nback to the main thread");
	printThreadStates(threadList);
	return EXIT_SUCCESS;
}
