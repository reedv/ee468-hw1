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
#include <sys/types.h>
#define BUFFER_SIZE 80  // Includes the space for terminating null
#define ARR_SIZE 80
#define MAX_PIPES 1  // for now
#define MAX_CHILDS MAX_PIPES+1

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
void launchProg(char **args, int background){
	 int err = -1;

	 if((pid=fork()) == -1){
		 printf("Child process could not be created\n");
		 return;
	 }

	 if(pid==0){
		/************************************
		 * Child Logic
		 * **********************************/
		// We set the child to ignore SIGINT signals (we want the parent
		// process to handle them with signalHandler_int)
		signal(SIGINT, SIG_IGN);

		// We set parent=<pathname>/simple-c-shell as an environment variable
		// for the child
		//setenv("parent",getcwd(currentDirectory, 1024),1);

		// If we launch non-existing commands we end the process
		if (execvp(args[0],args) == err){
			printf("Command not found");
			kill(getpid(),SIGTERM);
		}
	 }

	 /************************************
	 * Parent Logic
	 * **********************************/
	 // If the process is not requested to be in background, we wait for
	 // the child to finish.
	 if (background == 0){
		 waitpid(pid, NULL, 0);
	 }else{
		 // In order to create a background process, the current process
		 // should just skip the call to wait. The SIGCHILD handler
		 // signalHandler_child will take care of the returning values
		 // of the childs.
		 printf("Process created with PID: %d\n",pid);
	 }
}


/**
* Method used to manage pipes.
*/
void pipeHandler(char * args[]){
	// File descriptors
	int filedes[2]; // pos.0 output, pos.1 input of the pipe
	int filedes2[2];

	int num_cmds = 0;

	char *command[256];

	pid_t pid;

	int err = -1;
	int end = 0;

	// Variables used for the different loops
	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;

	// First we calculate the number of commands (they are separated
	// by '|')
	while (args[l] != NULL){
		if (strcmp(args[l],"|") == 0){
			num_cmds++;
		}
		l++;
	}
	num_cmds++;

	// Main loop of this method. For each command between '|', the
	// pipes will be configured and standard input and/or output will
	// be replaced. Then it will be executed
	while (args[j] != NULL && end != 1){
		k = 0;
		// We use an auxiliary array of pointers to store the command
		// that will be executed on each iteration
		while (strcmp(args[j],"|") != 0){
			command[k] = args[j];
			j++;
			if (args[j] == NULL){
				// 'end' variable used to keep the program from entering
				// again in the loop when no more arguments are found
				end = 1;
				k++;
				break;
			}
			k++;
		}
		// Last position of the command will be NULL to indicate that
		// it is its end when we pass it to the exec function
		command[k] = NULL;
		j++;

		// Depending on whether we are in an iteration or another, we
		// will set different descriptors for the pipes inputs and
		// output. This way, a pipe will be shared between each two
		// iterations, enabling us to connect the inputs and outputs of
		// the two different commands.
		if (i % 2 != 0){
			pipe(filedes); // for odd i
		}else{
			pipe(filedes2); // for even i
		}

		pid=fork();

		if(pid==-1){
			if (i != num_cmds - 1){
				if (i % 2 != 0){
					close(filedes[1]); // for odd i
				}else{
					close(filedes2[1]); // for even i
				}
			}
			printf("Child process could not be created\n");
			return;
		}
		if(pid==0){
			// If we are in the first command
			if (i == 0){
				dup2(filedes2[1], STDOUT_FILENO);
			}
			// If we are in the last command, depending on whether it
			// is placed in an odd or even position, we will replace
			// the standard input for one pipe or another. The standard
			// output will be untouched because we want to see the
			// output in the terminal
			else if (i == num_cmds - 1){
				if (num_cmds % 2 != 0){ // for odd number of commands
					dup2(filedes[0],STDIN_FILENO);
				}else{ // for even number of commands
					dup2(filedes2[0],STDIN_FILENO);
				}
			// If we are in a command that is in the middle, we will
			// have to use two pipes, one for input and another for
			// output. The position is also important in order to choose
			// which file descriptor corresponds to each input/output
			}else{ // for odd i
				if (i % 2 != 0){
					dup2(filedes2[0],STDIN_FILENO);
					dup2(filedes[1],STDOUT_FILENO);
				}else{ // for even i
					dup2(filedes[0],STDIN_FILENO);
					dup2(filedes2[1],STDOUT_FILENO);
				}
			}

			if (execvp(command[0],command)==err){
				kill(getpid(),SIGTERM);
			}
		}

		// CLOSING DESCRIPTORS ON PARENT
		if (i == 0){
			close(filedes2[1]);
		}
		else if (i == num_cmds - 1){
			if (num_cmds % 2 != 0){
				close(filedes[0]);
			}else{
				close(filedes2[0]);
			}
		}else{
			if (i % 2 != 0){
				close(filedes2[0]);
				close(filedes[1]);
			}else{
				close(filedes[0]);
				close(filedes2[1]);
			}
		}

		waitpid(pid,NULL,0);

		i++;
	}
}

/**
* Method used to handle the commands entered via the standard input
*/
int commandHandler(char * args[]){
	int i = 0;
	int j = 0;

	int fileDescriptor;
	int standardOut;

	int aux;
	int background = 0;

	// TODO: comment out unneeded conditionals to see if we can get MINIMAL functionality (just pipes)
	// 		 Only keep the pieces you can understand and dont keep anything beyond single basic commands
	//       (with options) and pipes (|).
	// 'exit' command quits the shell
	if(strcmp(args[0],"exit") == 0) exit(0);
 	// 'clear' command clears the screen
	else if (strcmp(args[0],"clear") == 0) system("clear");
//	// 'cd' command to change directory
//	else if (strcmp(args[0],"cd") == 0) changeDirectory(args);
	else{
		// If none of the preceding commands were used, we invoke the
		// specified program. Detect if piped execution was used.
		while (args[i] != NULL){
			// If '|' is detected, piping was solicited, and we call
			// the appropriate method that will handle the different
			// executions
			if (strcmp(args[i],"|") == 0){
				pipeHandler(args);
				return 1;
			}
			i++;
		}
		// We launch the program with our method, indicating if we
		// want background execution or not
		launchProg(args,background);

		/**
		 * For the part 1.e, we only had to print the input that was not
		 * 'exit', 'pwd' or 'clear'. We did it the following way
		 */
		//	i = 0;
		//	while(args[i]!=NULL){
		//		printf("%s\n", args[i]);
		//		i++;
		//	}
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
        /* special 'exit' command entered, exit shell */
        if (strcmp(args[0], "exit" )==0) exit(0);

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
//        /*
//         * TODO: parse thru args to find '|'s and use these positions to create array of
//         * array of args and thier options. Then can use something like
//         * execvp(args[i][0], args[i]) in run_commands.
//         * */
//		int num_args = pipe_count + 1;  // total num of args
//		int	num_options = nargs - num_args - pipe_count + 1;  // +1 since also needs to hold arg of the options
//		char piped_args[num_args][num_options][ARR_SIZE]; // piped_args[i][0]=ith arg, piped_args[i][j>0]=jth option of ith arg
//		// init.all strings of piped_args to be empty strings (or '\n'?)
//		for(i=0; i<num_args; i++){
//			for(j=0; j<num_options; j++){
//				strcpy(piped_args[i][j], "\0");
//#ifdef DEBUG
//				printf("**In main: piped_args init.: piped_args[%d][%d] = %s\n", i, j, piped_args[i][j]);
//#endif
//			}
//		}
//		/*
//		 * Parse thru args to find '|'s and use these positions to create array of
//         * array of args as well as thier options.
//		 * */
//		for(i=0, j=0, k=0; i<nargs; i++){  // Assuming we have already caught empty commands and the first arg is valid
//			if(strcmp(args[i], "|")==0) {
//				// start next arg and options
//				j++;
//				k=0;
//			} else {
//				// add arg or options to indexed array of strings in piped_args
//				strcpy(piped_args[j][k], args[i]);
//				k++;
//			}
//		}
//
//#ifdef DEBUG
//		// check contents of piped_args
//		for(i=0; i<num_args; i++){
//			for(j=0; j<num_options; j++){
//				printf("**In main: piped_args[%d][%d] = %s\n", i, j, piped_args[i][j]);
//			}
//		}
//#endif
//
//
//		pid = fork();  // returns a value of 0 in the child process and returns the
//					   // child's process ID in the parent process.
//		if (pid){  /* The parent */
//#ifdef DEBUG
//			printf("**In main: Waiting for child (%d)\n", pid);
//#endif
//			pid = wait(ret_status);
//			/*
//			 * This is a simplified version of waitpid, and is used to wait
//			 * until any one child process terminates.
//			 */
//#ifdef DEBUG
//			printf("**In main: Child (%d) finished\n", pid);
//#endif
//		}
//
//		else{  /* The child executing the command */
////				if( execvp(args[0], args)) {
////					/*
////					 * The execvp function is similar to execv, except that it searches the directories
////					 * listed in the PATH environment variable to find the full file name of a file
////					 * from filename if filename does not contain a slash.
////					 *
////					 * This function is useful for executing system utility programs, because it looks
////					 * for them in the places that the user has chosen. Shells use it to run the
////					 * commands that users type.
////					 */
////					/*
////					 * The  execv(const char *filename, char *const argv[]) function executes the file named
////			         * by filename as a new process image.
////                     *
////					 * The argv argument is an array of null-terminated strings that is used to provide
////					 * a value for the argv argument to the main function of the program to be executed.
////					 * The last element of this array must be a null pointer. By convention, the first
////					 * element of this array is the file name of the program sans directory names. , for full
////					 * details on how programs can access these arguments.
////					 */
////					// notify if errors
////					puts(strerror(errno));
////					exit(127);
////			}
//		}

    }
    return 0;
}


