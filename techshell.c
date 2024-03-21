/*
* Name(s): Jonah Hoffman, Tim Farley
* Date: 2/21/2024
* Description: A small shell that can handle basic shell commands and I/O redirections. All of the commands in the document work.
*
*
*/
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// helper functions
void error() {
	fprintf(stderr, "Error %d (%s)\n", errno, strerror(errno));
}

// unix commands execvp couldn't handle
void shell_exit() {
	exit(0);
}

void shell_cd(char** arg) {
	if (arg[1] != NULL) {
		// if destination directory, go there
		if (strcmp(arg[1], "~") == 0) chdir(getenv("HOME")); 
		else if (chdir(arg[1]) != 0) error();
	} else {
		// if no destination given, go home
		chdir(getenv("HOME"));
	}
}


char* CommandPrompt() { // Display current working directory and return user input
	static char input[999]; // Holds the user input
	char cwd[999];

	if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s$ ", cwd); // Prints the CWD and $
	fgets(input, 999, stdin);
	if (strlen(input) > 0 && input[strlen(input)-1] == '\n') input[strlen(input)-1] = '\0'; // Trims the \n off of the input
	
	return input;
}

// initialize ShellCommand structure
struct ShellCommand {
	char* command;
	char** args;
	char* i_file;
	char* o_file;
	int append_o;

};

struct ShellCommand ParseCommandLine(char* input) { // Process the user input (As a shell command)
	// initialize vars
	struct ShellCommand c;
	int argc = 0;
	memset(&c, 0, sizeof(struct ShellCommand)); // Resets the memory of the variable

	// tokenize command
	char *token;
	token = strtok(input, " ");
	c.command = token;

	while (token != NULL) {
		// I/O redirection
		if (strcmp(token, "<") == 0) {
			token = strtok(NULL, " ");
			c.i_file = token;
			token = strtok(NULL, " ");
			continue;
		} else if (strcmp(token, ">") == 0) {
			token = strtok(NULL, " ");
			c.o_file = token;
			c.append_o = 0;
			token = strtok(NULL, " ");
			continue;
		} else if (strcmp(token, ">>") == 0) {
			token = strtok(NULL, " ");
			c.o_file = token;
			c.append_o = 1;
			token = strtok(NULL, " ");	
			continue;
		}
		
		// if token is an argument, append here
		c.args = realloc(c.args, (argc + 1) * sizeof(char*));
		c.args[argc++] = token;
		token = strtok(NULL, " ");
	}
	

	c.args = realloc(c.args, (argc + 1) * sizeof(char*));
	c.args[argc] = NULL;

	return c; 

}

void ExecuteCommand(struct ShellCommand c) { //Execute a shell command
        if (strcmp(c.command, "cd") == 0) {
		return shell_cd(c.args);
	}

	if (strcmp(c.command, "exit") == 0) {
		return shell_exit();
	}

	// creates a child process to execute commands
        pid_t child = fork();
        if (child == 0) {
     		// I/O redirection
                if (c.i_file != NULL){
                        FILE* infile = fopen(c.i_file, "r");
                        dup2(fileno(infile), 0);
                        fclose(infile);
                }
                if (c.o_file != NULL){
			if (c.append_o) {
				FILE* outfile = fopen(c.o_file, "a");
                        	dup2(fileno(outfile), 1);
                        	fclose(outfile);
			} else {
				FILE* outfile = fopen(c.o_file, "w");
                        	dup2(fileno(outfile), 1);
                        	fclose(outfile);
                	}
		}

          	// runs execvp and prints errors if needed
                execvp(c.command, c.args);
		error();
		exit(EXIT_FAILURE);

	// wait for child to finish before moving on
        } else {
                int status;
                waitpid(child, &status, 0);
        }
}



int main() {
    char* input;
    struct ShellCommand command;

    // repeatedly prompt the user for input
    for (;;)
    {
        input = CommandPrompt();
        // parse the command line
        command = ParseCommandLine(input);
        // execute the command
	ExecuteCommand(command);
    }
    exit(0);

}
