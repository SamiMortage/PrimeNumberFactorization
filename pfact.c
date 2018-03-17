#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <sys/wait.h>
#include "filter.h"
void pipe_checked(int pipefd[2]) {
	if (pipe(pipefd) == -1) {
		perror("pipe");
		exit(1);
	}
}

int fork_checked() {
	int ret = fork();
	if (ret == -1) {
		perror("fork");
		exit(1);
	}
	return ret;
}

void close_checked(int filedes) {
	if (close(filedes) == -1) {
		perror("close");
		exit(1);
	}
}

void write_checked(int fildes, const void *buf, size_t nbyte) {
    if (write(fildes, buf, nbyte) == -1) {
        perror("write");
        exit(1);
    }
}

void read_checked(int fildes, void *buf, size_t nbyte) {
    if (read(fildes, buf, nbyte) == -1) {
        perror("read");
        exit(1);
    }
}

int main(int argc, char **argv){
    char *endptr;
    if (argc != 2 || strtol(argv[1], &endptr, 10) < 2 || (*endptr != '\0')){
	fprintf(stderr, "Usage:\n\tpfact n\n");
	return 1;
    }
    int n = strtol(argv[1], NULL, 10);
    int factors_fd[2];
    int numfactors_fd[2];
    int first_child_fd[2];
    pipe(factors_fd); /**/	
    pipe(numfactors_fd);
    pipe(first_child_fd);
    int res_master = fork_checked();
    if(res_master == 0){
	/*First Child Process*/
	close_checked(numfactors_fd[0]);/*None of the children read from this pipe*/
	close_checked(factors_fd[0]); /*None of the children read from this pipe*/
	int children_pipe_fd[2]; /*Fild descriptors for child communication*/
		
	int oldpipe_fd[2];/*Keeps track of the previous pipe's fd*/
	for (int i = 0; i<2; i++){
	    /*Since this is the master's first child, the oldpipe
	    *file descriptors are copied from first_child_fd
	    */
	    oldpipe_fd[i] = first_child_fd[i];
	}
	int numfactors = 0; /*Keeps track of the # of factors*/
	int m = 2; /*Keeps track of current filter, which is 2 for the first child*/
		
	while(m <= n){
	    close_checked(oldpipe_fd[1]);
			
            if (m >= sqrt(n)){
	        /*It shouldn't filter*/
		int j;
    		while(read(oldpipe_fd[0], &j, sizeof(int)) != EOF){
		    if(n % j == 0 ){
		    	/*Increment numfactors and write factor to parent*/
		    	numfactors++;
		    	write_checked(factors_fd[1], &j, sizeof(int));
		    }
		}
		close_checked(oldpipe_fd[0]);
	        close_checked(factors_fd[1]);
		write_checked(numfactors_fd[1], &numfactors, sizeof(int));
		close_checked(numfactors_fd[1]);
		exit(0);	
	    }
	    if(n % m == 0){
		if(numfactors == 1){
		    /*There are 2 factors less than sqrt(n)*/
		    numfactors = -1;
		    write_checked(numfactors_fd[1], &numfactors, sizeof(int));
		    close_checked(numfactors_fd[1]);
		    close_checked(factors_fd[1]);
		    close_checked(oldpipe_fd[0]);
	            exit(0);
		}
		else{
		    write_checked(factors_fd[1], &m, sizeof(int));
		    numfactors++;	
		}
	    }
	    pipe_checked(children_pipe_fd);
	    int res_fork = fork_checked();
	    if(res_fork == 0){
		/*Child Process*/
		read_checked(children_pipe_fd[0], &m, sizeof(int));/*Update m*/
		for (int i = 0; i < 2; i++){
		    /*Update oldpipe to be the pipe it's parent wrote to*/ 
		    oldpipe_fd[i] = children_pipe_fd[i];
		}
	    }
	    else{
		/*Parent Process*/
		close_checked(factors_fd[1]); /*If there were any factors it already wrote it*/
		close_checked(numfactors_fd[1]); /*The last process is responsible to write this to master*/
		close_checked(children_pipe_fd[0]); /*Only writing to this pipe*/
		if (filter(m, oldpipe_fd[0], children_pipe_fd[1]) == 1){
		    fprintf(stderr, "filter function failed.");
		}
		close_checked(oldpipe_fd[0]);/*Done reading*/
		close_checked(children_pipe_fd[1]);/*Done writing*/
		int status;	
		wait(&status);
		if(WIFEXITED(status)){
		    exit(1 + WEXITSTATUS(status));
		}
	    }



	} 


    }
    else{
        /*Master Process*/
        close_checked(factors_fd[1]); /*Master won't need to write to this pipe*/
        close_checked(numfactors_fd[1]); /*Master won't need to write to this pipe*/
	close_checked(first_child_fd[0]); /*Master won't need to read from this pipe*/
		
	for(int i = 2; i <= n; i++){
	    /*Send the first child all numbers from 2 to n*/
	    write_checked(first_child_fd[1], &i, sizeof(int));
        }
        close_checked(first_child_fd[1]); /*Finished writing*/
        int numfactors;
	read(numfactors_fd[0], &numfactors, sizeof(int));
	if(numfactors == 0){
    	    printf("%d is prime\n", n);
	}
        else if(numfactors == -1 || numfactors > 2){
	    printf("%d is not the product of two primes\n", n);
	}
	else{
	    int factor1;
	    read(factors_fd[0], &factor1, sizeof(int));
	    if(numfactors == 2){
		int factor2;
		read(factors_fd[0], &factor2, sizeof(int));
		printf("%d %d %d\n", n, factor1, factor2);
	    }
	    else{
		if (sqrt(n) == factor1){
		    printf("%d %d %d\n", n, factor1, factor1);
		}
		else{
		    printf("%d is not the product of two primes\n", n);
	        }
	    }
        }	
	close_checked(factors_fd[0]); /*Read factors*/
        close_checked(numfactors_fd[0]); /*Read numfactors*/
	int status;
	wait(&status);
        if(WIFEXITED(status)){
	    printf("Number of filters = %d\n", WEXITSTATUS(status));
        }
    }
    return 0;
}
