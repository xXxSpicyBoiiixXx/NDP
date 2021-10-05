#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include "mergeSort.h"

#define LIMIT 10 

int main(int argc, char ** argv) {

	size_t len;	
	int idx, new_val;
	int ret = 1;	
	void(*f)(int [],size_t, size_t) = NULL; 
// 	uint64_t(*f)(int*, size_t) 	

	clock_t start, end; 
	double cpu_time_used; 

	if(argc < 2) { 
		usage(argv[0]);
		exit(EXIT_SUCCESS); 
	}
	
	len = atoll(argv[1]); 
		
	int * sortArray = malloc(sizeof(int)*len); 
	
	if (!sortArray) { 
		fprintf(stderr, "Could not allocate array\n"); 
		exit(EXIT_FAILURE); 
	}
	
	init_array(sortArray, len); 
	printf("Unsorted Attay: "); 
	print_array(sortArray, len); 
	printf("\n"); 

	start = clock(); 
	
	if(argv[2] != NULL) { 

	if(strcmp(argv[2], "merge") == 0) {
		f = mergeSort;
	}
}

	else{ 
		fprintf(stderr, "Unknown function type: %s\n", argv[2]); 
	}
	
	f(sortArray, 0, len - 1); 
	
	printf("Sorted Array: ");
	print_array(sortArray, len); 
	
	do { 

	end = clock();
	
	cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
	printf("\n"); 
	printf("computation time: %f", cpu_time_used);  
	printf("\nEnter index in the array you want to replace followed by the value: ");
	scanf("%d %d", &idx, &new_val); 
	sortArray[idx] = new_val;

	// Recomputation and printing

	start = clock(); 
	f(sortArray, 0, len - 1);
	print_array(sortArray, len); 
	
	}
	while(ret != 0);

	return 0;
}
