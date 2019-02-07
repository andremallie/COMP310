/*
 * The Tiny Torero Shell (TTSH)
 *
 * Add your top-level comments here.
 */

#define _XOPEN_SOURCE

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <signal.h>

#include "parse_args.h"
#include "history_queue.h"


static void handleCommand(char **args, int bg);
void runExternalCommand(char **args, int bg);
void parseAndExecute(char *cmdline, char **args);

void child_reaper(__attribute__ ((unused)) int sig_num) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}


int main(){ 
	char cmdline[MAXLINE];
	char *argv[MAXARGS];

	// register a signal handler for SIGCHLD
	struct sigaction sa;
	sa.sa_handler = child_reaper;
	sa.sa_flags = 0;
	sigaction(SIGCHLD, &sa, NULL);

	while(1) {
		// (1) print the shell prompt
		fprintf(stdout, "ttsh> ");  
		fflush(stdout);

		// (2) read in the next command entered by the user
		if ((fgets(cmdline, MAXLINE, stdin) == NULL) 
				&& ferror(stdin)) {
			clearerr(stdin);
			continue;
		}

		if (feof(stdin)) { /* End of file (ctrl-d) */
			fflush(stdout);
			exit(0);
		}

		//fprintf(stdout, "DEBUG: %s\n", cmdline);

		parseAndExecute(cmdline, argv);
	}

	return 0;
}

void parseAndExecute(char *cmdline, char **args) {
	// make a call to parseArguments function to parse it into its argv format
	int bg = parseArguments(cmdline, args);

	// determine how to execute it, and then execute it
	if (args[0] != NULL) {
		if (args[0][0] != '!')
			add_to_history(cmdline);
		handleCommand(args, bg);
	}
}

void handleCommand(char **args, int bg) {         
	// handle built-in directly
	if (strcmp(args[0], "exit") == 0) {
		printf("Goodbye!\n");
		exit(0);
	}
	
	else if (strcmp(args[0], "history") == 0) {
		print_history();
	}

	else if (args[0][0] == '!') {
		unsigned int cmd_num = strtoul(&args[0][1], NULL, 10);
		char *cmd = get_command(cmd_num);
		if (cmd == NULL)
			fprintf(stderr, "ERROR: %d is not in history\n", cmd_num);
		else {
			parseAndExecute(cmd, args);
		}
	}

	// handle external command with our handy function
	else
		runExternalCommand(args, bg);
}

void runExternalCommand(char **args, int bg) {
	pid_t cpid = fork();
	if (cpid  ==  0) {
		// child
		execvp(args[0], args);
		// if we got to this point, execvp failed!
		fprintf(stderr , "ERROR: Command not found\n");
		exit(63);
	}
	else if (cpid > 0) {
		// parent
		if (bg) {
			// Quick check if child has returned quickly.
			// Don't block here if child is still running though.
			waitpid(cpid, NULL, WNOHANG);
		}
		else {
			// wait here until the child really finishes
			waitpid(cpid, NULL, 0);
		}
	}
	else {
		// something went wrong with fork
		perror("fork");
		exit(1);
	}
}
