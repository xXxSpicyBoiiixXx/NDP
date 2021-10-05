#include <stdlib.h> 
#include <stdio.h> 

#define LIMIT 100 

/*
 *
 */ 
void usage(char *prog) { 
	printf("Usage: %s <n> [sorting algorithm]\n", prog); 
	printf("	<n>: length of array\n"); 
	printf("	sorting algorithm: merge sort |\n"); 
}

/*
 * 
 */ 
void init_array(int *a, int len) { 
	for(int i = 0; i < len; i++) {
		a[i] = rand() % LIMIT;
	}
}

/*
 * 
 */ 

void print_array(int *arr, int len) {
	for(int i = 0; i < len; i++) { 
		printf("%d ", arr[i]); 
	}
	printf("\n"); 
}
