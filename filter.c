#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "filter.h"
int filter(int m, int read_fd, int write_fd){	
	int return_val = 0;
	int n;
	while (read(read_fd, &n, sizeof(int)) != EOF){
		if (n % m != 0){
			if(write(write_fd, &n, sizeof(int)) == -1){
				perror("write");
				return_val = 1;
			}
		}
	}
	return return_val;
}
