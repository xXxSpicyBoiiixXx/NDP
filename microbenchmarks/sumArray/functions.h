/*
Header file for functions 
*/

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

void print_array(int *arr, size_t len);
void init_array(int *arr, size_t len);  
void usage(char *prog); 

uint64_t naive_sum(int *arr, size_t len); 
uint64_t naive_min(int *arr, size_t len);
uint64_t naive_max(int *arr, size_t len); 

#endif
