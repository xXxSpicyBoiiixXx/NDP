#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <stdint.h>

#define LIMIT 100

#define DEBUG 1

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

static uint64_t smarter_sum(int * arr, size_t n) {
	// TODO: fill me in
	return 0;
}


static inline void usage(char * prog) {
	printf("Usage: %s <n>\n", prog);
	printf("   <n>: length of array\n");
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


	printf("Initial sum: %llu\n", naive_sum(array, len)); 
	
	do { 
#if DEBUG==1
		print_array(array, len);
#endif
		printf("Enter index in the array you want to replace followed by the value: ");
		ret = scanf("%d %d", &idx, &new_val); 
		array[idx % len] = new_val;
		printf("Recomputed sum: %llu\n", naive_sum(array, len)); 
	}	
	while (ret != 0);

	return 0;
}

