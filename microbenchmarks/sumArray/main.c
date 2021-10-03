#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include <limits.h>
#include "functions.h"

#define LIMIT 100

#define DEBUG 1

/*typedef struct node { 
	int data; 
	struct node* left; 
	struct node* right; 
};

node* newNode(int data) {
	struct node* node = (struct node*)malloc(sizeof(struct node));
	
	node->data = data; 
	
	node->left = NULL;
	node->right = NULL; 
	
	return(node); 
}


struct node *createTree(int * arr, size_t len) {
	
	node *left_node;
	node *right_node;
	
	node *summation_node; 
	
	int sum_array[len/2]; 
	
	for(int i = 0, i < len, i++) { 
		
		leaf_node = (node*)malloc(sizeof(node)); 
		leaf_node -> data; 
		
		i++; 
		if(i != len) 
		right_node = (node*)malloc(sizeof(node)); 
		right_node -> data;

		int j = i--;
		
		sum_array[j] = left_node + right_node;

	}

	createTree(sum_array, size_t len/2); 

}*/


static uint64_t swaping_naive_sum(int * arr, int swap_idx, int value) {
	
	uint64_t diff = 0; 
		
	diff = arr[swap_idx]; 
	arr[swap_idx] = value; 
	diff = diff - value; 

	return diff; 
}


static uint64_t smarter_sum(int * arr, size_t n) {

	//int sumLeft, sumRight, sum; 
 	
	//struct node *root = createTree(arr, 0, end_idx); 
	
	int left_sum = 0;
	int right_sum = 0;
	int  sum; 
		
	return sum;
}


/*
const char * function_option(char ** argv) {
	 
	if(argv[2] != NULL { 
		if(strcmp(argv[2], "sum") == 0) { 
			f = naive_sum; 
		} else if 
	}
	
}
*/

int main(int argc, char ** argv) {
	
	size_t len;
	int idx, new_val; 
	int ret = 1;
	uint64_t(*f)(int*, size_t);

	if (argc < 2) {
		usage(argv[0]);
		exit(EXIT_SUCCESS);
	}


	len = atoll(argv[1]);

	int * array = malloc(sizeof(int)*len);
	if (!array) {
		fprintf(stderr, "Could not allocate array\n");
		exit(EXIT_FAILURE);
	}
	

	init_array(array, len); 
	printf("Randomly Generated Array: ");
	print_array(array, len); 

    // user specified a function type
    if (argv[2] != NULL) {
	if(strcmp(argv[2], "sum") == 0) {
	    f = naive_sum; 
        } else if (strcmp(argv[2], "max") == 0) {
            f = naive_max;
        } else if (strcmp(argv[2], "min") == 0) {
            f = naive_min;
        } else if(strcmp(argv[2], "smarter sum") == 0) {
	    f = smarter_sum; 
	} else {
            fprintf(stderr, "Unknown function type: %s\n", argv[2]);
            exit(EXIT_FAILURE);
        }
  }
	printf("Initial result: %llu\n", f(array, len)); 

	do { 
#if DEBUG==1
		print_array(array, len);
#endif
		printf("Enter index in the array you want to replace followed by the value: ");
		ret = scanf("%d %d", &idx, &new_val); 
		array[idx % len] = new_val;
		printf("Recomputed function: %llu\n", f(array, len)); 
	}	
	while (ret != 0);


	return 0;
}

