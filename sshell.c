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
 * Currently works onyl for args w/out options and they cannot be seperated
 * by '|' tokens. E.g. "$ ls wc" works, but "$ ls | wc" does not.
 * */
void run_commands(int nargs, char *args[]){
	printf("Entered run_commands\n");

	// going thru args from left to right
	int i;
	for( i=0; i<nargs-1; i++){  // the very last arg will be eval. outside the loop
		/*
		 * In this loop, we continuously a child to eval the next
		 * arg and pipe the child's stdout to the stdin of the parent,
		 * which spawns another child to eval this stdin with the next arg,
		 * and so on up until the very last arg.
		 * */
		int pd[2];
		pipe(pd);

		// child logic
		if (!fork()) {

			// remap stdout to write to parent
			dup2(pd[1], 1);
			close(pd[0]);
			execlp(args[i], args[i], (char*)NULL);

			perror("exec");
			abort();
		}

		// parent logic

		// remap output from child to stdin
		dup2(pd[0], 0);
		close(pd[1]);
	}

	execlp(args[i], args[i], (char*)NULL);

	perror("exec");
	abort();
}

int main(int argc, char *argv[], char *envp[]){
    char buffer[BUFFER_SIZE];
    char *args[ARR_SIZE];

    int *ret_status;
    size_t nargs;
    pid_t pid;
    
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
        printf("nargs=%d\n", nargs);
        int i;
        for(i=0; i<nargs; i++){
        	printf("args[%d] = %s\n", i, args[i]);
        }
#endif
 
        /* Nothing entered so prompt again */
        if (nargs==0) continue;
        /* special 'exit' command entered, exit shell */
        if (!strcmp(args[0], "exit" )) exit(0);

        //count number of pipes
        int pipe_count=0;
        for(i=0; i<nargs; i++){
        	if(strcmp(args[i], "|")==0) {
        		pipe_count++;
        	}
        }
#ifdef DEBUG
        printf("pipe_count=%d\n", pipe_count);

#endif
        run_commands(nargs, args);
//			pid = fork();  // returns a value of 0 in the child process and returns the
//						   // child's process ID in the parent process.
//			if (pid){  /* The parent */
//	#ifdef DEBUG
//				printf("Waiting for child (%d)\n", pid);
//	#endif
//				pid = wait(ret_status);
//				/*
//				 * This is a simplified version of waitpid, and is used to wait
//				 * until any one child process terminates.
//				 */
//	#ifdef DEBUG
//				printf("Child (%d) finished\n", pid);
//	#endif
//			}
//
//			else{  /* The child executing the command */
//				if( execvp(args[0], args)) {
//					/*
//					 * The execvp function is similar to execv, except that it searches the directories
//					 * listed in the PATH environment variable to find the full file name of a file
//					 * from filename if filename does not contain a slash.
//					 *
//					 * This function is useful for executing system utility programs, because it looks
//					 * for them in the places that the user has chosen. Shells use it to run the
//					 * commands that users type.
//					 */
//					// notify if errors
//					puts(strerror(errno));
//					exit(127);
//			}
//
//			}

    }    
    return 0;
}

