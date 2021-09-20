#include <stdio.h>
#include <stdlib.h>
#include "array.h"

int main() {
	struct ArrayData *array;
	struct ArrayData *sortedArray;

	array = initArray();
	sortedArray = initArray();
	
	int element;

	int prevElement, nextElement;

	int i, j, tmp;

	for (i = 0; i < 10; i++) {
		addElement(array, rand() % 100);
		
		//Commenting out for now, want to sort this array and then add another element. 
		element = getElement(array, i);
  		printf("%d\n", element);
	}

	printf("\n");

	for (i = 0; i < 10; i++){
		for(j = i + 1; j < 10; j++)
		{	
			prevElement = getElement(array, i);
			nextElement = getElement(array, j);
			
			if(nextElement < prevElement)
			{	
				element = getElement(array, i);
				tmp = element;
				prevElement = nextElement;
				nextElement = tmp;
			}
		
		}
		// addElement(sortedArray, prevElement);
	}
	
	for(i = 0; i < 10; i++)
	{	
		element = getElement(sortedArray, i);
		printf("%d\n", element);
	}
	
	

	free(array->pointer);
	free(sortedArray->pointer);
	free(array);
	free(sortedArray);
	return 0;
}
