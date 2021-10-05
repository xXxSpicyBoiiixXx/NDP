#include <stdio.h>
#include <stdlib.h>
#include "mergeSort.h"

#define LIMIT 10 

int main(int argc, char ** argv) {

	size_t len;	
	int idx, new_val;
	int ret = 1;	
	uint64_t(*f)(int*, size_t); 
	
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

	if(argv[2] != NULL) { 

	if(strcmp(argv[2], "merge")) == 0 {
		f = mergeSort(sortArray, 0, len - 1);
	} 

	else{ 
		fprintf(stderr, "Unknown function type: %s\n", argv[2]); 
	}

	do {
	printf("Enter index in the array you want to replace followed by the value: ");
	scanf("%d %d", &idx, &new_val); 
	sortArray[idx] = new_val;

	// Recomputation and printing
	mergeSort(sortArray, 0, len - 1);
	print_array(sortArray, len);
	}
	while(ret != 0);

	return 0;
}
