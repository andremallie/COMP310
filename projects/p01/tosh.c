/*
 * The Torero Shell (TOSH)
 *
 * Add your top-level comments here.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <readline/readline.h>


// TODO: add your function prototypes here as necessary


int main(){ 

	// TODO: add a call to signal to register your signal handler on 
	//       SIGCHLD here

	while(1) {
		// (1) read in the next command entered by the user
		char *cmdline = readline("tosh$ ");

		// NULL indicates EOF was reached, which in this case means someone
		// probably typed in CTRL-d
		if (cmdline == NULL) {
			fflush(stdout);
			exit(0);
		}

		fprintf(stdout, "DEBUG: %s\n", cmdline);

		// TODO: complete the following top-level steps
		// (2) parse the cmdline

		// (3) determine how to execute it, and then execute it

	}

	return 0;
}
