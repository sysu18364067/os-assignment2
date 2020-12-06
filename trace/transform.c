#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>

int main(){
	unsigned long pagesize = getpagesize();
	printf("%ld", pagesize);
} 
