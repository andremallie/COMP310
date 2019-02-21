/*
 * Title: tosh$
 * Authors: Andre Mallie and Cal Ferraro
 * Date: 2/20/19
 * Description: This program emulates a bash shell. It has support for external commands, pipes, and built in commands.
 * 		The progrmas basic operation is printing a shell promt, waiting for a command from the user, and then 
 *		exectuting that command. Once the command is executed the cycle restarts. Each function is described in more details below.
 */

#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>

#include "parse_args.h"
#include "history_queue.h"

int histCounter = -1;

static void handleCommand(char **args, int bg);
void runExternalCommand(char **args, int bg);
void parseAndExecute(char *cmdline, char **args);
int length(char* s);
void pipeCmd(char** arg1, char** arg2i, int bg);
void ioRedirect(char **args, int bg, int ioarg);

void child_reaper(__attribute__ ((unused)) int sig_num) {
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

/* Description: Main program loop. Prompt user, read commandline, execute command, reprompt.
 * 		This loop restarts after command execution.
 * Arguments: None
 * Primary Author: None
*/
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

/* Description:         This function takes in the user input to the command line and an empty args array.
 *			It then calls parseArguments(), which tokenizes the command line and plces the commands
 * 			into the args array.
 *
 *			It then checks to see if the command entered by the user should be added to the 
 *			history queue and then sends the command to handleCommand().
 * Arguments:           char **args:    empty array of args from the command line, ends in a null terminator
 *                      char *cmdline:	a string representing user input from the command line
 * Primary Author:      none
*/

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


/* Description:         This function takes in an arg array and an interger indicating if the command should be run in
 *			the foreground or background. The function then checks to see if the command from the user
 *			has any pipe or file I/O commands. if so then it handles the commands using their respective helper functions.
 * 			If the command does not match one of those, then the function checks to see if the command matches
 *			any of the built in commands:
 *				cd
 *				history
 *				exit
 *				!num
 *				!!
 *			If the command does not match any of these then it passes on its arguments to runExternalCommand().			
 * Arguments:           char **args:    array of args from the command line, ends in a null terminator
 *                      int bg:         integer indicating if the command is to be run in the background or not
 *                                              bg = 0  :       run in foreground
 *                                              bg = 1  :       run in background
 * Primary Author:      none
*/

void handleCommand(char **args, int bg) { 
	//Check for pipes or file I/O
	char ioFlag = 0;
	char pipeFlag = 0;
	int argCount = 0;
	int pipeInd = 0;
	int ioarg = 0;
	while(args[argCount] != NULL) {
		int l = length(args[argCount]);
		for(int i = 0; i<l; i++) {
			if(args[argCount][i] == '<' || args[argCount][i] == '>') {
				ioFlag = 1;
				ioarg = argCount;
				break;
			}
			else if(args[argCount][i] == '|' && pipeInd == 0) {
				pipeFlag = 1;
				pipeInd = argCount;
				break;
			}
		}
		argCount++;
	}	
    
	//handle built in commands
	if(pipeFlag) {
		char** arg1 = malloc(MAXLINE*sizeof(char*));
		char** arg2 = malloc(MAXLINE*sizeof(char*));
		for(int i = 0; i < pipeInd; i++) {
			arg1[i] = args[i];
		}
		arg1[pipeInd] = '\0';
		for(int j = 0; args[j] != NULL; j++) {
			arg2[j] = args[j+pipeInd+1];
		}
		pipeCmd(arg1, arg2, bg);
		free(arg1);
		free(arg2);
	}
	else if(ioFlag) {
		ioRedirect(args, 0, ioarg);
	}
	else if (strcmp(args[0], "exit") == 0) {
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

/* Description:         This function runs any command that does not fall under the list of built in commands or pipe and file I/O redirection. 
 *			It finds the executable path for the command and then executes the command using execv().
 *
 * Arguments:           char **args:    array of args from the command line, ends in a null terminator
 *                      int bg:         integer indicating if the command is to be run in the background or not
 *                                              bg = 0  :       run in foreground
 *                                              bg = 1  :      	run in background
 * Primary Author:      Andre Mallie
*/
void runExternalCommand(char **args, int bg) {
	pid_t cpid = fork();
	if(cpid == 0) {
		//child
		//Check to see if the cmdline can be accessed directly
		if(access(args[0],F_OK && X_OK) == 0) {
			execv(args[0], args);
		}
		//first try failed, search for the command
		else {
			char *pth = getenv("PATH");
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

/* Description:         This function takes in an array of args and counts how many args there are in the array.
 *
 * Arguments:           char *s: array of arg arguments
 * Primary Author:      Cal Ferraro
*/
int length (char* s) {
	int i = 0;
	while(s[i] != '\0')
		i++;
	return i;
}

/* Description:         This function takes two argument arrays, one before and one after the | call. In addition it takes in the
			integer bg, which indicates if a command is to be run in the background or not. 
			
			The function then closes the approperiate input/output in order to redirect the command input and outputs to each other.
			After doing so it cals handleCommand().
 * Arguments:           char **arg1:    array of args from the command line that are before the | call, ends in a null terminator
			char **arg2:	array of args from the command line that are after the | call, ends in a null terminator            
 *                      int bg:         integer indicating if the command is to be run in the background or not
 *                                              bg = 0  :       run in foreground
 *                                              bg = 1  :       run in background
 * Primary Author:      Cal Ferraro
*/
void pipeCmd(char** arg1, char** arg2, int bg) {
	pid_t pid;
	int p[2];
	pipe(p);
	if(pipe(p) < 0)
		perror("pipe");

	pid = fork();
	if(pid == 0) {
		close(1);
		close(p[0]);
		dup2(p[1], STDOUT_FILENO);

		handleCommand(arg1, bg);
		printf("ERROR: Failed to Execute!");
		exit(1);
	}
	else {
		pid = fork();
		if(pid == 0) {
			close(0);
			close(p[1]);
			dup2(p[0], STDIN_FILENO);
	
			handleCommand(arg2, bg);
		}
		else {
			int status;
			close(p[0]);
			close(p[1]);
			waitpid(pid, &status, 0);
		}
	}

//	if (pid1 > 0) {
//		if (bg) {
//			waitpid(pid1, NULL, WNOHANG);
//		}
//		else {
//			waitpid(pid1, NULL, 0);
//		}
//	}
//
//	if (pid2 > 0) {
//		if (bg) {
//			waitpid(pid2, NULL, WNOHANG);
//		}
//		else {
//			waitpid(pid2, NULL, 0);
//		}
//	}
	
//	close(p[0]);
//        close(p[1]);
}

/* Description: 	This function takes in an array of arguments representing a command entered by the user, along
 *			with an integer value indicating whether this operation should be run in the background.
 *			Lastly it takes in an integer value showing where the index of the first I/O file operator is.
 *			
 *			Using the input arguments the function loops through the args and determins what kind of redirect
 *			is needed. It then opens and closes the approperiate output/input and reroutes it to the argument 
 *			after the I/O file operator. 
 *
 *			The origional arg array is copied into a different arry with only the first command and a null 
 *			terminator. This is then run with execv(). 
 * Arguments:		char **args: 	array of args from the command line, ends in a null terminator
 *			int bg: 	integer indicating if the command is to be run in the background or not
 *						bg = 0	:	run in foreground
 *						bg = 1	:	run in background
 *			int ioarg:	integer indicating where the first I/O redirect command is
 * Primary Author:	Andre Mallie
*/
void ioRedirect(char **args, int bg, int ioarg) {
	//Define new arg array to hold the command to be executed without any of the 
	//file I/O redirect infromation
	char **arg3 = malloc(MAXLINE*sizeof(char*));
	pid_t cpid = fork();
	if(cpid == 0) {
		//child
                // handle I/O redirect
                for(int i = 0; args[i] != 0; i++) {
                	//Check for stdout redirect only
                	if(strcmp(args[i], "1>") == 0) {
                        	int fid = open(args[i + 1], O_WRONLY | O_APPEND | O_CREAT, 0666);
                        	for (int i = 0; i < ioarg; i++) {
                                        arg3[i] = args[i];
                                }
                                arg3[ioarg] = '\0';
                                dup2(fid, 1);
                                close(fid);
                        }
			//Check for stderr redirect
                        if(strcmp(args[i], "2>") == 0) {
                                int fid = open (args[i + 1], O_WRONLY | O_APPEND | O_CREAT, 0666);
                                for (int i = 0; i < ioarg; i++) {
                                        arg3[i] = args[i];
                                }
                                arg3[ioarg] = '\0';
				dup2(fid, 2);
                                close(fid);
                        }
                        //Check for stdin redirect
                         if(strcmp(args[i], "<") == 0) {
                                int fid = open (args[i + 1], O_RDONLY);
                                for (int i = 0; i < ioarg; i++) {
                                        arg3[i] = args[i];
                                }
                                arg3[ioarg] = '\0';
				dup2(fid, 0);
                                close(fid);
                        }
                }

                //Check to see if the cmdline can be accessed directly
                if(access(arg3[0],F_OK && X_OK) == 0) {
                        execv(arg3[0], arg3);
                }
                //first try failed, search for the command
                else {
                        char *pth = getenv("PATH");
                        char *full_pth = malloc(MAXLINE*sizeof(char));
                        char *pth_copy = malloc(MAXLINE*sizeof(char));
                        char *token = malloc(MAXLINE*sizeof(char));
                        pth_copy = strndup(pth, MAXLINE);
                        token = strtok(pth_copy, ":");
                        while(token != NULL) {
                                sprintf(full_pth, "%s/%s", token, arg3[0]);
                                if(access(full_pth,F_OK && X_OK) == 0) {
                                        execv(full_pth, arg3);
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
