/*
*	Program to execute a pipeline
*
*	Author: Leo Juneblad (c19lsd)
*
* Version: 1.0
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#define READ_END 0
#define WRITE_END 1
#define MAX 1024

char **resize_array(char **array, int size, int length);
int **create_pipes(int com_amount);
void fork_process(int com_amount, int **pipes, char **commands);
void child(int line, int com_amount, char **commands, int **pipes);
void parent(int com_amount, int **pipes);
void dup_child(int com_amount, int line, int **pipes);
void close_pipes(int **pipes, int com_amount);
void free_memory(char **array, int **pipes, int com_amount);


int main(int argc, char *argv[]) {

	FILE *file;
	char **commands;
	int com_amount = 0;
	int **pipes;

	//Check if the user provided a file and open it if they did.
	if(argc == 1) {
		file = stdin;
	}

	else if(argc == 2) {
		if((file = fopen(argv[1], "r")) == NULL) {
			perror(argv[1]);
			exit(EXIT_FAILURE);
		}
	}

	//If too many arguments were given.
	else {
		fprintf(stderr, "usage: ./mexec [FILE]\n");
		exit(EXIT_FAILURE);
	}

	//Allocate default size of array.
	if((commands = malloc(1 * sizeof(char*))) == NULL) {
		perror("Malloc 1 failed");
		exit(EXIT_FAILURE);
	}

	//Copy the contents of the given file line by line into dynamically allocated
	//array using resize_array.
	char temp[MAX];
	while(fgets(temp, MAX, file) != NULL) {
		commands = resize_array(commands, com_amount, MAX);
		strcpy(commands[com_amount], temp);
		com_amount++;
	}

	//If file was given close it.
	if(argc == 2) {
		if(fclose(file)) {
			perror("close file:");
			exit(EXIT_FAILURE);
		}
	}

	pipes = create_pipes(com_amount);

	fork_process(com_amount, pipes, commands);

	free_memory(commands, pipes, com_amount);

	return 0;
}

/*
*	Reallocates more space in the given array of strings.
*
*	@array: The array to be resized.
*	@size: The size new size of the array.
*	@length: The length of the string in the new space in the array.
*
*	Returns: A pointer to the reallocated array.
*
*/
char **resize_array(char **array, int size, int length) {
	//Reallocate the size of the array.
	if((array = realloc(array, (size + 1) * sizeof(char*))) == NULL) {
		perror("realloc array: ");
		exit(EXIT_FAILURE);
	}

	//Allocate space for the string in the array.
	if((array[size] = malloc(length * sizeof(char))) == NULL) {
		perror("malloc line:");
		exit(EXIT_FAILURE);
	}

	return array;
}

/*
*	Creates an array of pipes.
*
*	@com_amount: The number of processes that need pipes.
*
*	Returns: An array of pipes.
*
*/
int **create_pipes(int com_amount) {
	int **pipes;
	//Allocate space the array.
	if((pipes = malloc((com_amount - 1) * sizeof(int*))) == NULL) {
		perror("malloc pipes");
		exit(EXIT_FAILURE);
	}
	//Allocate space for the read/write end of the pipes and creates the pipes.
	for(int i = 0; i < com_amount - 1; i++) {
		pipes[i] = malloc(2 * sizeof(int));
		if(pipe(pipes[i]) != 0) {
			perror("pipes");
			exit(EXIT_FAILURE);
		}
	}

	return pipes;
}

/*
*	Runs fork to create a given amount of child processes.
*
*	@com_amount: The size of the array of commands to be executed.
*	@pipes: The pipes that processes will use to communicate.
*	@commands: The array of commands to be executed.
*
*	Returns: Nothing if succesfull.
*
*/
void fork_process(int com_amount, int **pipes, char **commands) {
	int pid;

	//Creates a process for each command.
	for(int i = 0; i < com_amount; i++) {
		//printf("h\n");
		pid = fork();
		if(pid < 0) {
			perror("Fork failed");
			exit(EXIT_FAILURE);
		}
		//If the fork is succesfully execued
		else if(pid == 0) {
			child(i, com_amount, commands, pipes);
			exit(0);
		}
	}
	parent(com_amount, pipes);
}

/*
*	Run in the child process if the fork is succesfull. Gets the command to be
*	executed and executes it.
*
*	@line: The line in the array of commands that contains the command to be run.
*	@com_amount: The size of the array of commands to be executed.
*	@commands: The array containing the commands to be executed.
*	@pipes: The array of pipes.
*
*	Returns: Nothing if succesfull.
*
*/
void child(int line, int com_amount, char **commands, int **pipes) {
	char **command;

	if((command = malloc(1 * sizeof(char*))) == NULL) {
		perror("malloc:");
		exit(EXIT_FAILURE);
	}

	int i = 1;
	int j = 0;

	//Go through the line char by char and allocate memory for it.
	char c = commands[line][j];
	while(c != '\n') {
		//If a space is found memory is allocated in the array for the word
		if(isspace(c) && c != '\0'){
			command = resize_array(command, i, j);
			i++;
		}
		j++;
		c = commands[line][j];
	}
	//Space at the end of the array for NULL.
	command = resize_array(command, i + 1, 1);

	//Split up arguments and store them as seperate strings.
	i = 0;
	char *token = strtok(commands[line], " \n");
	while(token != NULL) {
		command[i] = token;
		i++;
		token = strtok(NULL, " \n");
	}
	command[i] = NULL;

	dup_child(com_amount, line, pipes);

	close_pipes(pipes, com_amount);

	//Execute the command
	if(execvp(command[0], command) < 0) {
		perror(command[0]);
		exit(EXIT_FAILURE);
	}
}

/*
*	Runs in the parent process if the fork is succesfull. Waits for child
*	processes.
*
*	@com_amount: The size of the array of commands to be executed.
*	@pipes: The array of pipes.
*
*	Returns: Nothing if succesfull.
*
*/
void parent(int com_amount, int **pipes) {
	int status;

	close_pipes(pipes, com_amount);

	//Waiting and error checking child processes.
	for(int i = 0; i < com_amount; i++) {
		if(wait(&status) == -1) {
			perror("wait: ");
			exit(EXIT_FAILURE);
		}

		if(status != 0) {
			exit(EXIT_FAILURE);
		}
	}
}

/*
*	Runs in the parent process if the fork is succesfull. Waits for child
*	processes.
*
*	@com_amount: The size of the array of commands to be executed.
*	@line: The pipe to be dup2'd.
*	@pipes: The array of pipes.
*
*	Returns: Nothing if succesfull.
*
*/
void dup_child(int com_amount, int line, int **pipes) {
	//If only one command is given no duping is needed.
	if(com_amount != 1) {
		//Check first and last child.
		if(line != com_amount - 1) {
			dup2(pipes[line][WRITE_END], STDOUT_FILENO);
		}
		if(line != 0) {
			dup2(pipes[line - 1][READ_END], STDIN_FILENO);
		}
	}
}

/*
*	Closes all pipes in the given array of pipes.
*
*	@pipes: The array of pipes.
*	@com_amount: The number of processes that have used pipes.
*
*	Returns: Nothing if succesfull.
*
*/
void close_pipes(int **pipes, int com_amount) {
	for(int i = 0; i < com_amount - 1; i++) {
		close(pipes[i][0]);
		close(pipes[i][1]);
	}
}

/*
*	Closes all pipes in the given array of pipes.
*
*	@array: The array of commands to be executed.
*	@pipes: The array of pipes.
*	@com_amount: The number of commands in the array and the number of pipes +1.
*
*	Returns: Nothing if succesfull.
*
*/
void free_memory(char **array, int **pipes, int com_amount) {
	for(int i = 0; i < com_amount; i++) {
		free(array[i]);
	}
	free(array);

	for(int i = 0; i < com_amount - 1; i++) {
		free(pipes[i]);
	}
	free(pipes);
}
