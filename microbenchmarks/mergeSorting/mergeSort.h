#ifndef MERGESORT_H_
#define MERGESORT_H_

//inline void usage(char * prog); 
void init_array(int * a, int len); 
void usage(char *prog);

void mergeSort(int * arr, size_t l, size_t r);
void merge(int * arr, int l, int m, int r);
void print_array(int * arr, int len); 

#endif
