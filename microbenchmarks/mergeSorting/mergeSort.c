#include <stdlib.h>
#include <stdio.h>
#include "mergeSort.h" 

#define LIMIT 10 


/*
 * 
 */
inline void usage(char * prog) {
	printf("Usage: %s <n> [sort algorithm]\n", prog); 
	printf("    <n>: length of array\n");
	printf("    sort algortihm: merge sort |\n");
}


/*
 * 
 */ 
inline void init_array(int * a, size_t len) { 
	for(int i = 0; i < len; i++) {
		a[i] = rand() % LIMIT;
	}
}


/*
 *
 */
void merge(int * arr, int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;
  
    int L[n1], R[n2];
  
    for (i = 0; i < n1; i++) {
        L[i] = arr[l + i];
    }

    for (j = 0; j < n2; j++) {
        R[j] = arr[m + 1 + j];
    }
  
    i = 0; 
    j = 0; 
    k = l; 

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        }
        else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }
  
    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }
  
    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void mergeSort(int * arr, int l, int r)
{
    if (l < r) {
        
        int m = l + (r - l) / 2;
  
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
  
        merge(arr, l, m, r);
    }
}

/*
 * 
 */
void print_array(int * arr, size_t len)
{
    for (int i = 0; i < len; i++)
        printf("%d ", arr[i]);
    printf("\n");
}
