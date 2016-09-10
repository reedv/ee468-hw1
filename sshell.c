/*
 *  This is a simple shell program from
 *  rik0.altervista.org/snippetss/csimpleshell.html
 *  It's been modified a bit and comments were added.
 *
 *  It doesn't allow misdirection, e.g., <, >, >>, or |
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>
#define BUFFER_SIZE 80  // Includes the space for terminating null
#define ARR_SIZE 80
#define READ 0  // filedes pipe sides
#define WRITE 1

pid_t pid;

#define DEBUG 1  /* In case you want debug messages */

void parse_args(char *buffer, char** args, 
                size_t args_size, size_t *nargs)
{
/* 
 * size_t data type is defined in the 1999 ISO C standard (C99).
 * It is used to represent the sizes of objects. size_t is the
 * preferred way to declare arguments or variables that hold the
 * size of an object.
 */
	// array to hold the seperated args
    char *buf_args[args_size]; /* You need C99.  Note that args_size
                                  is normally a constant. */
    char **cp;  /* This is used as a pointer into the string array, buf_args */
    char *wbuf;  /* String variable that has the command line */
    size_t i, j; 
    
    wbuf=buffer;
    buf_args[0]=buffer; 
    args[0] =buffer;
/*
 * Now 'wbuf' is parsed into the string array 'buf_args'
 *
 * The for-loop uses a string.h function
 *   char *strsep(char **stringp, const char *delim);
 *
 *   Description:  
 *   If *stringp = NULL then it returns NULL and does
 *   nothing else.  Otherwise the function finds the first token in
 *   the string *stringp, where tokens are delimited by symbols
 *   in the string 'delim'.  
 *
 *   In the example below, **stringp is &wbu, and 
 *   the delim = ' ', '\n', and '\t'.  So there are three possible 
 *   delimiters. 
 *
 *   So in the string " Aloha World\n", the spaces and "\n" are
 *   delimiters.  Thus, there are three delimiters.  The tokens
 *   are what's between the delimiters.  So the first token is
 *   "", which is nothing because a space is the first delimiter.
 *   The second token is "Aloha", and the third token is "World".
 *   
 *   The function will scan a character string starting from
 *   *stringp, search for the first delimiter.  It replaces
 *   the delimiter with '\0', and *stringp is updated to point
 *   PAST the token.  In case no delimiter was found, the
 *   token is taken to be the entire string *stringp, and *stringp
 *   is made NULL. Strsep returns a pointer to the token.
 *
 *
 *   Example:  Suppose *stringp -> " Aloha World\n"
 *
 *   The first time strsep is called, the string is "\0Aloha World\n",
 *   and the pointer value returned = 0.  Note the token is nothing.
 *
 *   The second time it is called, the string is "\0Aloha\0World\n",
 *   and the pointer value returned = 1  Note that 'Aloha' is a token.
 *
 *   The third time it is called, the string is '\0Aloha\0World\0', 
 *   and the pointer value returned is 7.  Note that 'World' is a token.
 *
 *   The fourth time it is called, it returns NULL.
 *
 *   The for-loop, goes through buffer starting at the beginning.
 *   wbuf is updated to point to the next token, and cp is
 *   updated to point to the current token, which terminated by '\0'.
 *   Note that pointers to tokens are stored in array buf_args through cp.
 *   The loop stops if there are no more tokens or exceeded the
 *   array buf_args.
 */   
    /* cp is a pointer to buff_args, so pointers to tokens are stored in array buf_args through cp */
    for(cp=buf_args; (*cp=strsep(&wbuf, " \n\t")) != NULL ;) {
    	/*
    	 * Successive calls to strsep move the pointer along the tokens separated
    	 * by delimiter (a string of specified delimiter tokens), returning the
    	 * address of the next token and updating string_ptr to point to the beginning
    	 * of the next token.
    	 */

    	/* if cp not at end of string and incrementing cp is at or beyond the buf_args */
        if ((*cp != '\0') && (++cp >= &buf_args[args_size]))
            break; 
    }

/* 
 * Copy 'buf_args' into 'args'
 */    
    for (j=i=0; buf_args[i]!=NULL; i++){ 
        if(strlen(buf_args[i])>0)  /* Store only non-empty tokens */
            args[j++]=buf_args[i];  /* store arg from buf_args into args and increment */
    }
    
    *nargs=j;
    args[j]=NULL;  // note args is ptr. to an array, so does not need to be returned
}


/*
* Method for launching a program.
*/
void commandRunner(char **args){
#ifdef DEBUG
	printf("**Entered commandRunner\n");
#endif
	 int err = -1;

	 if((pid=fork()) == -1){
		 printf("Child process could not be created\n");
		 return;
	 }

	 if(pid==0){
		/************************************
		 * Child Logic
		 * **********************************/
//		// Set child to ignore SIGINT signals
//		signal(SIGINT, SIG_IGN);

		if (execvp(args[0], args) == err){
			/*
			 * The execvp function is similar to execv, except that it
			 * searches the directories listed in the PATH environment variable
			 * to find the full file name of a file from filename if filename
			 * does not contain a slash.
			 *
			 * The argv argument is an array of null-terminated strings that
			 * is used to provide a value for the argv argument to the main
			 * function of the program to be executed. The last element of this
			 * array must be a null pointer. By convention, the first element of
			 * this array is the file name of the program sans directory names.
			 * , for full details on how programs can access these arguments.
			 *
			 * This function is useful for executing system utility programs, because
			 * it looks for them in the places that the user has chosen.
			 * Shells use it to run the commands that users type.
			 * */

			// If tried to launch non-existing commands, end process
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
	 }

	 /************************************
	 * Parent Logic
	 * **********************************/
	 // Wait for child to finish.
#ifdef DEBUG
	 printf("Process created with PID: %d\n",pid);
#endif
	 waitpid(pid, NULL, 0);
}


/**
* Method used to manage pipes.
*/
void pipeHandler(char * args[]){
#ifdef DEBUG
	printf("**Entered pipeHandler\n");
#endif
	// File descriptors
	int filedes[2]; // pos.0 output, pos.1 input of the pipe
	int filedes2[2];

	int num_cmds = 0;  // tracks commands for piped seq


	char *command[256];

//	pid_t pid;  // Have declared this as global, but kept this here in case something breaks down the line

	int err = -1;
	int end = 0;

	// Variables for loops
	int i = 0,  // tracks which commands we are at in a seq. of piped commands
		j = 0,  // tracks each arg between set of pipes
		k = 0,  // tracks each arg in an aux. array of string ptrs.for commands between pipes
		l = 0;  // tracks amount of pipes '|' in a command seq.

	// Calculate the number of commands (separated by '|')
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;
	const int last_cmd_index = num_cmds-1;

	// For each command between '|', configure pipes and set input/output.
	// Then execute
	while (args[j] != NULL && end != 1){
		k = 0;  // tracks args positions for each command
				// in the command[] array of string ptrs.

		// While the arg is not a pipe
		while (strcmp(args[j],"|") != 0){
			// Auxiliary array of pointers used to store the commands
			// for execution on each iteration
			command[k] = args[j];

			j++;
			// If no args would be found in a next iteration
			if (args[j] == NULL){
				end = 1;  // flag to not loop again
				k++;
				break;
			}
			k++;
		}
		// Last position, k, of the command needs to be NULL to indicate that
		// it is its end when we pass it to the exec function
		command[k] = NULL;
		j++;

		// Depending on if we are in one iteration or another, set different
		// descriptors for the pipes inputs and output. Thus, a pipe will be
		// shared between each two iterations, so the inputs and outputs of
		// the two different commands can communicate with each other.

		// odd i
		if (i % 2 != 0){
			pipe(filedes);
		// even i
		}else{
			pipe(filedes2);
		}

		pid=fork();

		if(pid==err){
			if (i != num_cmds - 1){
				// odd i
				if (i % 2 != 0){
					close(filedes[WRITE]);
				// even i
				}else{
					close(filedes2[WRITE]);
				}
			}
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){
			/************************
			 * Child Logic
			 * **********************/
			// If we are in very first command
			if (i == 0){
				/*This function copies the descriptor old to descriptor number new.*/
				dup2(filedes2[WRITE], STDOUT_FILENO);
			}
			// If processing last command, depending on whether it
			// is placed in an odd or even position, replace
			// the standard input for one pipe or another.
			// Don't change stdout, since want to see output in terminal
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[READ], STDIN_FILENO);
				}else{ // for even number of commands
					dup2(filedes2[READ], STDIN_FILENO);
				}

			// If we are in some command in the middle of pipe seq.,
			// have to use two pipes, one for input and one for
			// output. The position is also important in order to choose
			// which file descriptor corresponds to each input/output

		    	// odd i
			}else{
				if (i % 2 != 0){
					dup2(filedes2[READ], STDIN_FILENO);
					dup2(filedes[WRITE], STDOUT_FILENO);
				}else{ // even i
					dup2(filedes[READ], STDIN_FILENO);
					dup2(filedes2[WRITE], STDOUT_FILENO);
				}
			}

			if (execvp(command[0],command)==err){
#ifdef DEBUG
				printf("**In pipeHandler: execvp err\n");
#endif
				kill(getpid(), SIGTERM);
			}
		}

		/************************
		 * Parent Logic
		 * **********************/
		// close parent file descriptors
		if (i == 0){
			close(filedes2[WRITE]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){
				close(filedes[READ]);
			}else{
				close(filedes2[READ]);
			}
		}else{
			if (i % 2 != 0){
				close(filedes2[READ]);
				close(filedes[WRITE]);
			}else{
				close(filedes[READ]);
				close(filedes2[WRITE]);
			}
		}

		waitpid(pid, NULL, 0);
		/*
		 * The waitpid function is used to request status information from a child process
		 * whose process ID is pid. Normally, the calling process is suspended until the
		 * child process makes status information available by terminating.
		 * */

		i++;
	}
}

/**
* Method used to handle the commands entered via the standard input
*/
int commandHandler(char * args[]){
#ifdef DEBUG
	printf("**Entered commandHandler\n");
#endif
	int i = 0;
	int j = 0;

	/* Check for some common/basic commands */
	// 'exit' command quits shell
	if(strcmp(args[0],"exit") == 0) exit(0);
 	// 'clear' command clears screen
	else if (strcmp(args[0],"clear") == 0) system("clear");
	else{
		// Check if pipes used
		while (args[i] != NULL){
			if (strcmp(args[i],"|") == 0){
				pipeHandler(args);
				return 1;
			}
			i++;
		}
		commandRunner(args);  // (char *args[]) == (char **args), since c-arrays are ptrs.
	}

return 1;
}



int main(int argc, char *argv[], char *envp[]){
    char buffer[BUFFER_SIZE];
    char *args[ARR_SIZE];
    int i=0,
    	j=0,
		k=0;
    int *ret_status;
    size_t nargs;
    //pid_t pid;
    
    // main REPL
    while(1){
    	/* display prompt */
        printf("ee468>> ");
        /* Read in command line from stdin into buffer*/
        fgets(buffer, BUFFER_SIZE, stdin);
        /* Parse the command line arguments into array args */
        parse_args(buffer, args, ARR_SIZE, &nargs);
#ifdef DEBUG
			// check to see what args have been tokenized
			printf("**In main: nargs=%d\n", nargs);
			for(i=0; i<nargs; i++){
				printf("**In main: args[%d] = %s\n", i, args[i]);
			}
#endif
 
        /* Nothing entered so prompt again */
        if (nargs==0) continue;

        //Count number of pipes.
        int pipe_count=0;
        for(i=0; i<nargs; i++){
        	if(strcmp(args[i], "|")==0) {
        		pipe_count++;
        	}
        }
#ifdef DEBUG
		printf("**In main: pipe_count=%d\n", pipe_count);
#endif

		commandHandler(args);

    }
    return 0;
}


