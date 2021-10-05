#ifndef MERGESORT_H_
#define MERGESORT_H_

//inline void usage(char * prog); 
inline void init_array(int * a, size_t len); 
inline void usage(char *prog);

void mergeSort(int * arr, int l, int r);
void merge(int * arr, int l, int m, int r);
void print_array(int * arr, size_t len); 

#endif
