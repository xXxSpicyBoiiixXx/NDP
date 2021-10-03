#include <stdio.h>
#include <stdlib.h>
#include "mergeSort.h"

#define LIMIT 10 

int main(int argc, char ** argv) {

	size_t len;	
	int idx, new_val;
	int ret = 1;	
	
	/*
	if(argc < 2) { 
		usage(argv[0]);
		exit(EXIT_SUCCESS); 
	}
	*/

	len = atoll(argv[1]); 
	
	int * sortArray = malloc(sizeof(int)*len); 
	if (!sortArray) { 
		fprintf(stderr, "Could not allocate array\n"); 
		exit(EXIT_FAILURE); 
	}

	mergeSort(sortArray, 0, len - 1);
	
	printArray(sortArray, len);

	do {
	printf("Enter index in the array you want to replace followed by the value: ");
	scanf("%d %d", &idx, &new_val); 
	sortArray[idx] = new_val;

	// Recomputation and printing
	mergeSort(sortArray, 0, len - 1);
	printArray(sortArray, len);
	}
	while(ret != 0);

	return 0;
}
