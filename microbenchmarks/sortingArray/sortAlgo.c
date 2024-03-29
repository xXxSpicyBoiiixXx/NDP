#include <stdlib.h>
#include <stdio.h>
#include "sortAlgo.h" 

/*
 * Merge sorting 
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
 * Bubble Sort
 */ 

/*
void swap(int *element_1, int *element_2) {
	
	int temp = *element_1; 
	*element_1 = *element_2; 
	*element_2 = temp; 
}*/

void bubbleSort(int *arr, __attribute__((unused))  int place_holder, int len) { 

	for(int i = 0; i < len; i++) { 
	
		for(int j = 0; j < len - i - 1; j++) {	
			if(arr[j] > arr[j + 1]) {
				int temp = arr[j]; 
				arr[j] = arr[j + 1]; 
				arr[j+1] = temp;
			}
	
		}
	}
} 
