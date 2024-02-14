//define required libraries
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

//define global constants
#define MAX_NUM_ARGS 2049
#define MAX_COMMAND_SIZE 513
#define MAX_NUM_BACKGROUND_PROCESSES 1001

//define global variables for each of access during program
int backgroundFlag = 0;
int invalidFilePathFlag = 0;
int foregroundFlag = 0;

//define variables to track each background process and its number
pid_t backgroundProcesses[MAX_NUM_BACKGROUND_PROCESSES];
int numBackgroundProcesses = 0;

//define a variable to track the current termination status of the shell
int currentTerminationStatus = 0;

//termination status in code inspired by lecture "3.1 Processes" slides 23-26 inclusive cited 11/11
void checkBackgroundProcesses() {

	//initialize a variable to store the pid of each background process in the background processes array
	pid_t tempPid;

	//iterate through each background process and check its status
	for (int i = 0; i < numBackgroundProcesses; i++) {
		tempPid = waitpid(backgroundProcesses[i], &currentTerminationStatus, WNOHANG);
		
		//if the background process has changed state, check if it was exited normally or terminated by a signal
		if (tempPid != 0 && tempPid != -1) {
			if (WIFEXITED(currentTerminationStatus)) {
				printf("Background process PID %d has finished with exit status: %d.\n", backgroundProcesses[i], WEXITSTATUS(currentTerminationStatus));
				fflush(stdout);
			}
			else if (WIFSIGNALED(currentTerminationStatus)) {
				printf("Background process PID %d was terminated by signal: %d.\n", backgroundProcesses[i], WTERMSIG(currentTerminationStatus));
				fflush(stdout);
			}
		}
	}
}

//code inspired by functionality from "Exploration: Processes and I/O"; cited 11/10
void fileRedirection(char** args) {

	//reset flag before the function checks for an invalid path
	invalidFilePathFlag = 0;

	//initialize variables to track the existence and location of the input and output files
	int inputFileStatus;
	int outputFileStatus;
	char* inputFileLocation = NULL;
	char* outputFileLocation = NULL;

	//identify input and output file locations in the args array
	int index = 0;
	while (args[index] != NULL) {

		//set the input file location if a "<" is detected
		if (strcmp(args[index], "<") == 0) {
			args[index] = NULL;
			inputFileLocation = args[index + 1];
		}

		//set the output file location if a ">" is detected
		else if (strcmp(args[index], ">") == 0) {
			args[index] = NULL;
			outputFileLocation = args[index + 1];
		}

		index++;
	}

	//handle input file redirection and open the file for reading
	if (inputFileLocation != NULL) {
		inputFileStatus = open(inputFileLocation, O_RDONLY);
		//if the file is unable to be opened, set the flag to mark it as an invalid file path
		if (inputFileStatus == -1) {
			printf("No such file or directory exists.\n");
			fflush(stdout);
			invalidFilePathFlag = 1;
			return;
		}
		//redirect standard input to the input file
		else {
			dup2(inputFileStatus, STDIN_FILENO);
		}
		close(inputFileStatus);
	}

	//redirect standard input to /dev/null for background commands
	else if (backgroundFlag == 1) {
		inputFileStatus = open("/dev/null", O_RDONLY);
		//if the file is unable to be opened, set the flag to mark it as an invalid file path
		if (inputFileStatus == -1) {
			printf("No such file or directory exists.\n");
			fflush(stdout);
			invalidFilePathFlag = 1;
			return;
		}
		//redirect standard input to the input file
		else{
			dup2(inputFileStatus, STDIN_FILENO);
		}
		close(inputFileStatus);
	}

	//handle output file redirection and open the file for writing, creating, and truncating
	if (outputFileLocation != NULL) {
		outputFileStatus = open(outputFileLocation, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		//if the file is unable to be opened, set the flag to mark it as an invalid file path
		if (outputFileStatus == -1) {
			printf("No such file or directory exists.\n");
			fflush(stdout);
			invalidFilePathFlag = 1;
			return;
		}
		//redirect standard output to the output file
		else{
			dup2(outputFileStatus, STDOUT_FILENO);
		}
		close(outputFileStatus);
	}

	//redirect standard output to /dev/null for background commands
	else if (backgroundFlag == 1) {
		outputFileStatus = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0644);
		//if the file is unable to be opened, set the flag to mark it as an invalid file path
		if (outputFileStatus == -1) {
			printf("No such file or directory exists.\n");
			fflush(stdout);
     		invalidFilePathFlag = 1;
			return;
		}
		//redirect standard output to the output file
		else {
			dup2(outputFileStatus, STDOUT_FILENO);
		}
		close(outputFileStatus);
	}
}

//code inspired by functionality from "Exploration: Process API - Monitoring Child Processes" cited 11/10
void execFork(char** args) {

	//initialize a variable to store the pid of the spawned child 
	pid_t spawnPid = -1;

	//create a child process
	spawnPid = fork();
	switch (spawnPid) {

		//runs if fork failed
		case -1:
			printf("fork() failed!\n");
			fflush(stdout);
			exit(1);
			break;

		//runs the child process
		case 0:

			//execute i/o redirection
			fileRedirection(args);

			//if a valid file path was not given, exit the child process
			if (invalidFilePathFlag == 1) {
				exit(1);
			}
		
			//execute command with execvp
			execvp(args[0], args);
		
			//print an error if exec fails and the command is not found 
			printf("Command not found.\n");
			fflush(stdout);
			exit(1);

		//runs the parent process
		default:
		
			//if the command is running in the background and the shell is not in foreground-only mode, store its pid and print it
			if (backgroundFlag != 0 && foregroundFlag == 0) {
				backgroundProcesses[numBackgroundProcesses] = spawnPid;
				numBackgroundProcesses++;
				printf("Background process PID is: %d.\n", spawnPid);
				fflush(stdout);
			}

			//if the command is running in forground-only mode, wait for child process to terminate
			else {
				waitpid(spawnPid, &currentTerminationStatus, 0);
			}

			//check for completed background processes
			checkBackgroundProcesses();
			break;
	}
}

void execCommand(char** args, int argsCount) {

	//handle "exit" command. 
	//functionaly for group process killing cited from lecture slides 6-8 inclusive from "3.3 Signals" on 11/14 
    if (strcmp(args[0], "exit") == 0) {
        for (int i = 0; i < numBackgroundProcesses; i++) {
    
            //get the termination result of each background process
            int terminationStatus;
            pid_t terminationResult = waitpid(backgroundProcesses[i], &terminationStatus, WNOHANG);

            //check if the process is still runnning, and if so, terminate it
            if (terminationResult == 0) {
                kill(backgroundProcesses[i], SIGTERM);
            }
        }
        exit(0);
    }


	//handle "cd" command; code functionality for conditional statement inspired by "Exploration: Environment" cited 11/10
	else if (strcmp(args[0], "cd") == 0) {

		//change directory to the specified path based on the argument
		if (argsCount > 1) {
			chdir(args[1]);
		}

		//if the command is just "cd" without a specified path, return to home directory
		else {
			chdir(getenv("HOME"));
		}
	}

	//handle "status" command; code functionality conditional statement inspired by Lecture "3.1 Processes" on slides 23-26 (inclusive); cited 11/10 
	else if (strcmp(args[0], "status") == 0) {

		//print the exit value if the process was not running in the background and terminated normally 
		if (WIFEXITED(currentTerminationStatus) && backgroundFlag == 0) {
			printf("Exit value: %d.\n", WEXITSTATUS(currentTerminationStatus));
			fflush(stdout);
		}

		//print the termination signal if the process was not running in the background and was terminated by a signal
		else if (!WIFEXITED(currentTerminationStatus) && backgroundFlag == 0){
			printf("Terminated by signal. Exit value: %d.\n", WTERMSIG(currentTerminationStatus));
			fflush(stdout);
		}

		//if the background flag is set, exit with a value of 1
		else {
			currentTerminationStatus = 1;
			printf("Exit value: %d.\n", WTERMSIG(currentTerminationStatus));
			return;
		}

	}

	//create a child process with fork
	else {
		execFork(args);
	}

}

void expandCommand(char** args, int argsCount) {
	
	//initialize a a string to hold the pid of the shell
	char shellPid[256];
	sprintf(shellPid, "%d", getpid());

	//iterate through each character of each argument
	int i = 0;
	while (i < argsCount) {
		int j = 0;
		while (strlen(args[i]) > j) {
			//if two consecutive "$" are found, replace them with the pid of the shell by truncateing and then concatinating 
			if (args[i][j] == '$' && args[i][j + 1] == '$') {
				args[i][j] = '\0';
				snprintf(args[i], 256, "%s%s", args[i], shellPid);
			}
			j++;
		}
		i++;
	}

}

void tokenizeCommand(char** args, int* argsCount, char* token) {

	//tokenize input into the args array, track the number of arguments
	while (token != NULL) {
		strcpy(args[*argsCount], token);
		(*argsCount)++;
		token = strtok(NULL, " \n");
	}

	//used to indicate the end of the tokenized array
	args[*argsCount] = NULL;

}

void scanInput(char** args, char* storeBuffer, size_t size) {

	//use symbol as a prompt for each command line
	printf(": ");
	fflush(stdout);

	//store the buffer in the command line before tokenizing input
	ssize_t lineSize = getline(&storeBuffer, &size, stdin);
	char* token = strtok(storeBuffer, " \n");
	int argsCount = 0;
	tokenizeCommand(args, &argsCount, token);

	//expand the command by replacing the '$$' with pid of the shell
	expandCommand(args, argsCount);

	//skips comments or empty input
	if (argsCount == 0 || args[0][0] == '#') {
		return;
	}

	//if the argument ends in an '&', set the backgroundFlag so the program knows its a background process and reformat to be a program readable command
	backgroundFlag = 0;
	if (strcmp(args[argsCount - 1], "&") == 0) {
		backgroundFlag = 1;
		args[argsCount - 1] = NULL;
		argsCount--;
	}
	
	//execute commands 
	execCommand(args, argsCount);
	free(storeBuffer);

}

void catchSIGTSTP(int signo) {

	const char* enteringMsg = "\n: Entering foreground only mode . . . \n:\n";
	const char* exitingMsg = "\n: Exiting foreground only mode . . . \n:\n";

	//enters forground only mode if the flag is not set, then sets the flag so no background commands are able to run
	if (foregroundFlag == 0) {
		write(STDOUT_FILENO, enteringMsg, strlen(enteringMsg));
		foregroundFlag = 1;
	}

	//exits forground only mode if the flag is already set, then resets the flag so background commands can run
	else {
		write(STDOUT_FILENO, exitingMsg, strlen(exitingMsg));
		foregroundFlag = 0;
	}
}

//code functionality inspired by "Exploration: Signal Handling API" and "Exploration: Signals � Concepts and Types" cited 11/11
void establishSignals() {

	//initialze handlers for SIGINT and SIGTSTP
	struct sigaction SIGINT_action = { 0 };
	struct sigaction SIGTSTP_action = { 0 };

	//listens for CTRL + C; ignores the SIGINT signal, blocks all signals while the handler is running, clears the flags, and registers the handler
	sigfillset(&SIGINT_action.sa_mask);
	SIGINT_action.sa_flags = 0;
	sigaction(SIGINT, &SIGINT_action, NULL);

	//catches CTRL + Z with catchSIGTSTP function, blocks all signals while the handler is running, clears the flags, and registers the handler
	SIGTSTP_action.sa_handler = catchSIGTSTP;
	sigfillset(&SIGTSTP_action.sa_mask);
	SIGTSTP_action.sa_flags = 0;
	sigaction(SIGTSTP, &SIGTSTP_action, NULL);
}

int main() {

	while (1) {

		//establish the signals for CTRL + C and CTRL + Z
		establishSignals();
		
		//initialize variables that will store input from the command line
		char* storeBuffer = NULL;
		char** args = (char**)malloc(MAX_NUM_ARGS * sizeof(char*));
		for (int i = 0; i < MAX_NUM_ARGS; i++) {
			args[i] = (char*)malloc(MAX_COMMAND_SIZE * sizeof(char));
		}
		size_t size = 0;

		//scan input from the command line
		scanInput(args, storeBuffer, size);

		//attempt to free allocated memory
		for (int i = 0; i < MAX_NUM_ARGS; i++) {
			free(args[i]);
		}
		free(args);

	}

	return 0;
}
