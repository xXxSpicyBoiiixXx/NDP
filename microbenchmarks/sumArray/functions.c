#include <stdio.h> 
#include <stdlib.h> 
#include <string.h>
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/mman.h>
#include <stdint.h> 
#include <limits.h>
#include "functions.h"


/*
 * Function: print_array 
 * 
 * Description: This function prints an array to the console 
 */ 

void print_array(int *arr, size_t len) { 
	
	for(int i = 0; i < len; i++) { 
		printf("%d ", arr[i]); 
	}
	printf("\n"); 
}

/*
 * Function: usage 
 * 
 * Description: Prints out the use of the program
 */
void usage(char * prog) {
	
	printf("Usage: %s <n> [function]\n", prog);
	printf("<n>: length of array\n"); 
	printf("function: sum | max | min | smarter sum\n");  
}

/*
 * Function: init_array 
 *
 * Description: Fills a given array of a certain length with random numbers
 * 		between 1-100
 */
void init_array(int *arr, size_t len) { 
	
	for(int i = 0; i < len; i++) {
		arr[i] = rand() % 100;
	}
}


/*
 * Function: naive_sum 
 * 
 * Description: This function goes through an array one by one visiting each 
 *		index while adding the values to a varibale "sum" that is 
 *		returned to the program
 */
uint64_t naive_sum(int *arr, size_t len) { 

	uint64_t sum = 0; 
	
	for(int i = 0; i < len; i++) { 
		sum += arr[i];
	} 
	return sum; 
}


/* 
 * Fuction: naive_min 
 * 
 * Description: This function finds the minimum number in a given array 
 */ 
uint64_t naive_min(int *arr, size_t len) { 
		
	uint64_t min = INT_MAX; 
	
	for(int i = 0; i < len; i++) { 
		if(arr[i] < min) { 
			min = arr[i]; 
		}
	}
	return min; 
}

/*
 * Function: naive_max 
 * 
 * Description: This function find the maximum number in a given array
 */
uint64_t naive_max(int *arr, size_t len) {
	
	uint64_t max = 0; 

	for(int i = 0; i < len; i++) { 
		if(arr[i] > max) { 
			max = arr[i]; 
		}
	}
	return max;
}


