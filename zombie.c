#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h> // for wait macros etc
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

int main(int argc, char** argv)
{
	int f = fork();
	if (f < 0) {
		printf("fork failed: %s\n", strerror(errno));
		return -1; // ERROR!
	}
	
	if (f > 0) {
		printf("parent pid=%d, child pid=%d\n", getpid(), f);
		while (1);
	}
}
