#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NUM_THREADS 5

void *print_hello(void *threadid)
{
	long tid;
	tid = (long) threadid;
	printf("Hello World! It's me, thread #%ld!\n", tid);

	pthread_exit(NULL); // same as:	return 0;
}

int main(int argc, char *argv[])
{
	pthread_t threads[NUM_THREADS];
	int rc;
	long t;
	for(t=0; t<NUM_THREADS; t++)
	{
		printf("In main: creating thread %ld\n", t);
		rc = pthread_create(&threads[t], NULL, print_hello, (void *)t);
		if (rc)
		{
			printf("ERROR in pthread_create(): %s\n", strerror(rc));
			exit(-1);
		}
	}

	/* Last thing that main() should do */
	pthread_exit(NULL);	// DIFFERENT from:	return 0; (!)
}
