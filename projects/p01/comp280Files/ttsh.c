/*
 * The Tiny Torero Shell (TTSH)
 *
 * Add your top-level comments here.
 */

#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <signal.h>

#include "parse_args.h"
#include "history_queue.h"

int histCounter = -1;

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
		fprintf(stdout, "tosh$ ");  
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
		if (args[0][0] != '!') { 
			add_to_history(cmdline);
			histCounter++;
		}
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

	else if (args[0][0] == '!' && args[0][1] != '!') {
		unsigned int cmd_num = strtoul(&args[0][1], NULL, 10);
		char *cmd = get_command(cmd_num);
		if (cmd == NULL)
			fprintf(stderr, "ERROR: %d is not in history\n", cmd_num);
		else {
			parseAndExecute(cmd, args);
		}
	}
	else if (strcmp(args[0], "!!") == 0) {
		char *cmd = get_command(histCounter);
		if(histCounter == -1 || cmd == NULL)
			fprintf(stderr, "ERROR: no previous arguments in history\n");
		else {
			parseAndExecute(cmd,args);
		}
	}
	else if (strcmp(args[0], "cd") == 0) {
		if (args[1] == NULL) {
			chdir("/");
		}
		else {
			char *path = args[1];
			int change = chdir(path);	
			if (change == -1) {
				fprintf(stderr, "ERROR: The filepath %s may be incorrect, please try again\n", args[1]);
			}
		}
	}
	else	
		//Handle external commands
		runExternalCommand(args, bg);
}

void runExternalCommand(char **args, int bg) {
	pid_t cpid = fork();
	if(cpid == 0) {
		//child
		char *pth = getenv("PATH");
		//Check to see if the cmdline can be accessed directly
		if(access(args[0],F_OK && X_OK) == 0) {
			execv(args[0], args);
		}
		//first try failed, search for the command
		else {
			char *full_pth = malloc(MAXLINE*sizeof(char));
			char *pth_copy = malloc(MAXLINE*sizeof(char));
			char *token = malloc(MAXLINE*sizeof(char));
			pth_copy = strndup(pth, MAXLINE);
			token = strtok(pth_copy, ":");
			while(token != NULL) {
				sprintf(full_pth, "%s/%s", token, args[0]);
				if(access(full_pth,F_OK && X_OK) == 0) {
					execv(full_pth, args);
				}
				//Next Token
				token = strtok(NULL, ":");
			}
			
		}
		///if we got to this point, execv failed!
		fprintf(stderr, "ERROR: Command not found\n");
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
