#include <stdio.h>
#include <stdlib.h>
#include "array.h"

int main() {
	struct ArrayData *array;
	array = initArray();

	int element;

	int i;
	for (i = 0; i < 5000; i++) {
		addElement(array, rand() % 50000);
		element = getElement(array, i);
  		printf("%d\n", element);
	}

	free(array->pointer);
	free(array);
	return 0;
}
