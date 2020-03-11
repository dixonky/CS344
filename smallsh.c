/*******************************************************************************
* ** Author: Kyle Dixon
* ** Date: 5/21/2019-
* ** Descriptions: smallsh program (Block 3)
* ** 	
* ****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h> 	//for getpid()
#include <sys/stat.h>	//for mkdir() 
#include <sys/types.h>	//for mkdir()
#include <signal.h>		//for signal handlers
#include <fcntl.h>
#include <sys/wait.h>

//Global Constraints
#define CHAR_BUFFER 256
#define MAX_LENGTH 2048
#define MAX_ARG 512
#define PERMISSIONS 0666

//Global Variables
int signalFlagBackground = 1;

//Prototypes
void userInput(char*[], int*, char[], char[], int);
void execute(char*[], int*, struct sigaction, int*, char[], char[]);
void catchSIGTSTP(int);
void printStatus(int);


//Main Function
int main() {
	int i;
	int pid = getpid();
	int flagContinue = 0;
	int exitStatus = 0;
	int flagBackground = 0;

	char inputFile[CHAR_BUFFER] = "";
	char outputFile[CHAR_BUFFER] = "";
	char* input[MAX_ARG];
	for (i=0; i<MAX_ARG; i++) 
	{
		input[i] = NULL;
	}

	//Signal Handlers adapted from http://man7.org/linux/man-pages/man2/sigaction.2.html && lectures
	//http://www.cs.kent.edu/~ruttan/sysprog/lectures/signals.html
	struct sigaction sigint = {0};		//create null handler for sigint
	sigint.sa_handler = SIG_IGN;		//ignore the signal
	sigfillset(&sigint.sa_mask);		//initialize the whole set to block other signals https://www.gnu.org/software/libc/manual/html_node/Blocking-for-Handler.html
	sigint.sa_flags = 0;				//not planning on setting any flags
	sigaction(SIGINT, &sigint, NULL);	//not planning on setting any flags

	struct sigaction tstp = {0};		//create null handler for tstp
	tstp.sa_handler = catchSIGTSTP;		//get the info from the function into the handler
	sigfillset(&tstp.sa_mask);			//initialize the whole set to block other signals https://www.gnu.org/software/libc/manual/html_node/Blocking-for-Handler.html
	tstp.sa_flags = 0;					//not planning on setting any flags
	sigaction(SIGTSTP, &tstp, NULL);	//not planning on setting any flags

	while (flagContinue == 0) 
	{
		userInput(input, &flagBackground, inputFile, outputFile, pid);	//Get user input

		if (input[0][0] == '#' || input[0][0] == '\0') 		//check user input against saved commands
		{
			continue;		//do nothing if the first character is blank or a comment symbol
		}
		else if (strcmp(input[0], "exit") == 0) 
		{
			flagContinue = 1;	//set the continue flag and exit the while loop
		}
		else if (strcmp(input[0], "cd") == 0) 
		{
			if (input[1]) 		//check for a second argument to change to a specific dir
			{
				if (chdir(input[1]) == -1) 	//attempt to change to the user specified dir 
				{
					printf("Error: Dir not found\n");
					fflush(stdout);		//report failure and flush out stdout to continue
				}
			} 
			else 	//if there is no second argument, go home
			{
				chdir(getenv("HOME")); //adapted from https://www.tutorialspoint.com/c_standard_library/c_function_getenv.htm
			}
		}
		else if (strcmp(input[0], "status") == 0) 
		{
			printStatus(exitStatus); //print the saved exit status
		}
		else 	//if not a saved command, pass to the exec family of commands
		{
			execute(input, &exitStatus, sigint, &flagBackground, inputFile, outputFile);
		}

		for (i=0; input[i]; i++) //clear the input files before continuing to get the user input again
		{
			input[i] = NULL;
		}
		flagBackground = 0;
		inputFile[0] = '\0';
		outputFile[0] = '\0';
	} 
	return 0;
}


//user input Function
	//gets the user input and checks contents for commands
void userInput(char* correctedInput[], int* flagBackground, char input[], char output[], int pid){
	int i, j;
	char userInput[MAX_LENGTH];

	printf(": ");			//prompt to get user input
	fflush(stdout);			//clear stdout
	fgets(userInput, MAX_LENGTH, stdin); //store the char stream into user input

	int flagNewline = 0;			//remove the newline character via checking each char until one is found
	for (i=0; !flagNewline && i<MAX_LENGTH; i++) 
	{
		if (userInput[i] == '\n') 
		{
			userInput[i] = '\0';
			flagNewline = 1;
		}
	}
	if (!strcmp(userInput, "")) 	//check for blank input
	{
		correctedInput[0] = strdup("");		//set the first index value as a ptr to blank
		return;
	}

	const char delim[2] = " ";				//adapted from https://www.tutorialspoint.com/c_standard_library/c_function_strtok.htm
	char *symbol = strtok(userInput, delim); //break apart input into series of tokens, check tokens for symbols matching commands
	for (i=0; symbol; i++) 
	{
		if (!strcmp(symbol, "&")) 
		{
			*flagBackground = 1;
		}
		else if (!strcmp(symbol, ">")) 
		{
			symbol = strtok(NULL, delim);
			strcpy(output, symbol);
		}
		else if (!strcmp(symbol, "<")) 
		{
			symbol = strtok(NULL, delim);
			strcpy(input, symbol);
		}
		else 
		{
			correctedInput[i] = strdup(symbol);	//if the symbol doesn't represent a command, save a new copy of it (as a ptr) in correctedInput

			for (j=0; correctedInput[i][j]; j++) //search for the $$ symbol and replace with the pid 
			{
				if (correctedInput[i][j] == '$' && correctedInput[i][j+1] == '$') 
				{
					correctedInput[i][j] = '\0';
					snprintf(correctedInput[i], CHAR_BUFFER, "%s%d", correctedInput[i], pid);
				}
			}
		}
		symbol = strtok(NULL, delim);
	}
}


//Execute Function
	//
void execute(char* userInput[], int* childExitStatus, struct sigaction sa, int* flagBackground, char inputArray[], char outputArray[]) {
	int input, output;
	int copy;

	// Adapted from Lecture 3.1 && 3.4
	pid_t spawnPid = -5;
	spawnPid = fork();			//fork the child, will write into the pipe
	switch (spawnPid) {
		case -1:{
			perror("Hull Breach!\n");
			exit(1);
			break;}
		case 0:{
			sa.sa_handler = SIG_DFL;
			sigaction(SIGINT, &sa, NULL);

			if (strcmp(inputArray, ""))	// Adapted from Lecture 3.4
			{
				input = open(inputArray, O_RDONLY); //open the fifo for reading
				if (input == -1) 
				{
					perror("Error: Cannot open input file\n");
					exit(1);
				}
				copy = dup2(input, 0);
				if (copy == -1) 
				{
					perror("Error: Cannot assign input file\n");
					exit(2);
				}
				fcntl(input, F_SETFD, FD_CLOEXEC);
			}

			if (strcmp(outputArray, "")) // Adapted from Lecture 3.4
			{
				output = open(outputArray, O_WRONLY | O_CREAT | O_TRUNC, PERMISSIONS); //read only, create the path as a filename if it doesn't exist, attempt to truncate the file if it does exist
				if (output == -1) //report failure and exit with status 1
				{
					perror("Error: Cannot open input file\n");
					exit(1);
				}
				copy = dup2(output, 1);  //set output to get stdout, adapted from https://www.geeksforgeeks.org/dup-dup2-linux-system-call/
				if (copy == -1)	//report failure and exit with status 2
				{
					perror("Error: Cannot copy input file\n");
					exit(2);
				}
				fcntl(output, F_SETFD, FD_CLOEXEC); //change the file desciptor to NOT remain open, for potential race conditions
			}
			
			if (execvp(userInput[0], (char* const*)userInput))	//attempt to execute the passed in user input
			{
				printf("%s Error: No file or directory\n", userInput[0]);	//report failure and exit with status 2
				fflush(stdout);
				exit(2);
			}
			break; }
		
		default:{
			if (*flagBackground && signalFlagBackground)	//check the background flags to run in background
			{
				pid_t pid = waitpid(spawnPid, childExitStatus, WNOHANG); //return immediately if no child has exited
				printf("background pid is %d\n", spawnPid);
				fflush(stdout);
			}
			else 
			{
				pid_t pid = waitpid(spawnPid, childExitStatus, 0);	//wait until the child process has exited
			}	
			while ((spawnPid = waitpid(-1, childExitStatus, WNOHANG)) > 0) 
			{
				printf("child %d terminated\n", spawnPid);
				printStatus(*childExitStatus);	//send the child exit status (via ptr) to be printed
				fflush(stdout);
			}
			break;}
	}
}


//Catch signal TSTP Function
	//when the signal is called, a string is passed based on the flag
void catchSIGTSTP(int signo) {
	if (signalFlagBackground == 1) 
	{
		char* s = "Entering foreground-only mode (& is now ignored)\n"; //from directions
		write(1, s, 49);	//pass the string back into the signal handler
		fflush(stdout);	
		signalFlagBackground = 0;
	}
	else 
	{
		char* s = "Exiting foreground-only mode\n";	//from directions
		write(1, s, 29); //pass the string back into the signal handler
		fflush(stdout);
		signalFlagBackground = 1;
	}
}


//Print the exit status function
	//compares the child exit status (passed in) using macros https://www.gnu.org/software/libc/manual/html_node/Process-Completion-Status.html
void printStatus(int status) {
	if (WIFEXITED(status)) //returns a non-zero value if the child process terminated normally
	{
		printf("exit value %d\n", WEXITSTATUS(status));	//return the childs exit status
	} 
	else 
	{
		printf("terminated by signal %d\n", WTERMSIG(status));	//if not normal, return the signal number
	}
}


