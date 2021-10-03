#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>
#include <limits.h>

#define LIMIT 100

#define DEBUG 1

struct node { 
	int data; 
	struct node* left; 
	struct node* right; 
};

static struct node* newNode(int data) {
	struct node* node = (struct node*)malloc(sizeof(struct node));
	
	node->data = data; 
	
	node->left = NULL;
	node->right = NULL; 
	
	return(node); 
}

// leaf nodes are the values of trees
static struct createTree((int) *arr, int start_idx, int end_idx) {
	if(start_idx == end_idx)
		return NULL;

	/*int mid_idx = (start_idx + end_idx)/2;
	struct node *root = newNode(arr[mid_idx]);

	root->left = createTree(arr, start_idx, mid_idx-1); 
	root->right = createTree(arr, mid_idx+1, end_idx); 
	*/

	struct node *root = newNode; 

	for(start_idx, start_idx < end_idx, start_idx+=2) { 
		if(start_idx >= end_idx)
			return NULL; 

		struct node *createNode1 = newNode(arr[start_idx]); 
		struct node *createNode2 = newNode(arr[start_idx+1]); 
	
		struct node *sumNode = newNode(createNode1.data + createNode2.data); 
		sumNode -> left = createNode1; 
		sumNode -> right = createNode2; 

	}
	
	
/*	for(int start_idx, start_idx < end_idx, start_idx++) {
	
	struct node *createNode1 =  arr[start_idx]; 
	// there maybe an out of index here sometime.. 
	struct node *createNode2 =  arr[start_idx + 1];
	 }
		
	root
*/	
	return 0; 
}

static void print_array(int * arr, size_t len) {
	for (int i = 0; i < len; i++) {
		printf("%d ", arr[i]);
	}
	printf("\n");
}

static uint64_t naive_sum(int * arr, size_t n) {
	uint64_t sum = 0;

	for (int i = 0; i < n; i++) { 
		sum += arr[i];
	}
	return sum;
}

static uint64_t swaping_naive_sum(int * arr, int swap_idx, int value) {
	
	uint64_t diff = 0; 
		
	diff = arr[swap_idx]; 
	arr[swap_idx] = value; 
	diff = diff - value; 

	return diff; 
}

static uint64_t naive_min(int * arr, size_t n) {
	uint64_t min = INT_MAX;

	for (int i = 0; i < n; i++) { 
		if (arr[i] < min) 
            min = arr[i];
	}
	return min;
}

static uint64_t naive_max(int * arr, size_t n) {
	uint64_t max = 0;

	for (int i = 0; i < n; i++) { 
		if (arr[i] > max) 
            max = arr[i];
	}
	return max;
}

static uint64_t smarter_sum(int * arr, size_t n) {

	//int sumLeft, sumRight, sum; 
 	
	//struct node *root = createTree(arr, 0, end_idx); 
	
	int left_sum = 0;
	int right_sum = 0;
	int  sum; 
	
	
	
	
	
	
	return sum;
}


static inline void usage(char * prog) {
	printf("Usage: %s <n> [fun]\n", prog);
	printf("   <n>: length of array\n");
    printf("   fun: sum | max | min | smarter sum\n");
}

static inline void init_array(int * a, size_t len) {
	for (int i = 0; i < len; i++) {
		a[i] = rand() % LIMIT;
	}
}

int main(int argc, char ** argv) {
	
	size_t len;
	int idx, new_val; 
	int ret = 1;
    uint64_t(*f)(int*, size_t) = naive_sum;

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

	init_array(array, len);

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

