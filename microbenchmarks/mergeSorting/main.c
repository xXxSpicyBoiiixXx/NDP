#include <stdio.h>
#include <stdlib.h>
#include "mergeSort.h"

int main() {

	int sortArray[10];
	
	int i;
	
	int num1, num2;
		
	for(i = 0; i < 10; i++) {
		sortArray[i] = rand();
	}

	mergeSort(sortArray, 0, 9);
	
	printArray(sortArray, 10);

	do {
	printf("Enter placement in the array you want to replace followed by the value");
	scanf("%d %d", &num1, &num2); 
	sortArray[num1] = num2;
	mergeSort(sortArray, 0, 9);
	printArray(sortArray, 10);
	}
	while(num1 > 0 && num1 < 9);

	return 0;
}
