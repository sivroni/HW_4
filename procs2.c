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
	// arguments for the programs executed
	char* date_argv[] = { "/bin/date", "-u", NULL };
	char* ls_argv[] = { "/bin/ls", "-l", NULL };
	char* np_argv[] = { "numprint", "5", NULL };
	
	char buf[1024];
	char op[1024];
	
	// read line from stdin
	while (fgets(buf, 1024, stdin) != NULL) {
		assert(sscanf(buf, "%s", op) == 1);	// parse first word of line
	
		char** args;
	
		// find operation to execute, set args accordingly
		if (!strcmp(op, "date"))
			args = date_argv;
		else if (!strcmp(op, "ls"))
			args = ls_argv;
		else if (!strcmp(op, "np"))
			args = np_argv;
		else {
			printf("Unknown command\n");
			continue;
		}
		
		int f = fork();
		if (f < 0) {
			printf("fork failed: %s\n", strerror(errno));
			return -1;
		}
		
		if (f == 0) {
			// inside son process - execute with args
			execv(args[0], args);
			printf("execv failed: %s\n", strerror(errno));
			return -1;
		} else {
			// inside parent process - wait for son to finish, then continue
			int status;
			wait(&status);
		}
	}
	
	return 0;
}
