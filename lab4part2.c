/*============================================================================
 Name        : threadedTask.c
 Author      : Yangdi Lu
 Version     : Fall 2017
 Copyright   : Your copyright notice
 Description : This code creates a simple real time task of known priority and FIFO scheduling.
               Generally tasks run in an infinite loop
               however we use loop a finite loop for convenience in labs.
 ============================================================================*/

//#define _GNU_SOURCE //if you define the _GNU_SOURCE here, you do not need to, cnanot, add a symbol for Cross Compiler in Eclispse
 // if you add the symbol in Eclipse, you do not need to, cannot define the symbol here.


#if !defined(LoopDuration)
#define LoopDuration    10  /* How long to output the signal, in seconds */
#endif

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include "MyRio.h"
#include "DIO.h"

extern NiFpga_Session myrio_session;


#define MY_PRIORITY (49) /* we use 49 as the PRREMPT_RT use 50
                            as the priority of kernel tasklets
                            and interrupt handler by default */

#define MAX_SAFE_STACK (8*1024) /* The maximum stack size which is
                                   guaranteed safe to access without
                                   faulting */

#define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */

void stack_prefault(void) {

        unsigned char dummy[MAX_SAFE_STACK];

        memset(dummy, 0, MAX_SAFE_STACK);
        return;
}

void* tfun1(void*n){
    NiFpga_Status status;
    status = MyRio_Open();
    if (MyRio_IsNotSuccess(status))
    {
        return status;
    }

	struct timespec t;
	struct sched_param param;
	int interval = 200000000; // Determines the time period of the task

	// required for looping for a set time
	time_t currentTime;
	time_t finalTime;

	/* Declare ourself as a real time task */
	param.sched_priority = MY_PRIORITY;
	if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
			perror("sched_setscheduler failed");
			exit(-1);
	}

	/* Lock memory */
	if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
			perror("mlockall failed");
			exit(-2);
	}

	/* Pre-fault our stack */
	stack_prefault();

	clock_gettime(CLOCK_MONOTONIC ,&t);

	/* start after one second */
	t.tv_sec++;

	// Normally, the main function runs a long running or infinite loop.
	// A finite loop is used for convenience in the labs.

	time(&currentTime);
	finalTime = currentTime + LoopDuration;

	uint32_t led = DOLED30;
	uint8_t val = 0;

	while (currentTime < finalTime){
			/* wait until next shot */
			clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);

			/* do the stuff */

			NiFpga_WriteU8(myrio_session, led,((uint8_t) val%2));
			printf("Hello! This is thread: %u Priority:%d\n", (int)pthread_self(), param.sched_priority);
			printf("val: %d\n", val);

			val++;

			/* calculate next shot */
			t.tv_nsec += interval;

			while (t.tv_nsec >= NSEC_PER_SEC) {
				   t.tv_nsec -= NSEC_PER_SEC;
					t.tv_sec++;
			}
			time(&currentTime);
	}  //end of while (currentTime)
	return 0;


	////////////////////////////////////////////////////////////////////////////////////////////
	status = MyRio_Close();

    return status;
}


int main(int argc, char* argv[])
{
   pthread_t tid1, tid2, tid3;
   cpu_set_t cpus;
   // Force the program to run on one cpu,
   CPU_ZERO(&cpus); //Initialize cpus to nothing clear previous info if any
   CPU_SET(0, &cpus); // Set cpus to a cpu number zeor here

   if (sched_setaffinity(0, sizeof(cpu_set_t), &cpus)< 0)
      perror("set affinity");

   pthread_create(&tid1, NULL, tfun1, NULL);
   pthread_create(&tid2, NULL, tfun1, NULL);
   pthread_create(&tid3, NULL, tfun1, NULL);

   pthread_join(tid1, NULL);
   pthread_join(tid2, NULL);
   pthread_join(tid3, NULL);

   return 0;

}
