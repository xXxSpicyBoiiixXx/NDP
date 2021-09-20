#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "array.h"

struct ArrayData *initArray() {
	struct ArrayData *newArray = malloc(sizeof(struct ArrayData));
	newArray->pointer = calloc(1000, sizeof(int));
	newArray->size = 1000;
	newArray->counter = 0;
	return newArray;
}

void resizeArray(struct ArrayData* array) {
	int newSize = (array->size * sizeof(int)) * 2;
	array->pointer = realloc(array->pointer, newSize);
	fflush (stdout);
	array->size *= 2;  // This is the number of elements, don't multiply by sizeof
}

int addElement(struct ArrayData *array, int number) {
	if (array->counter >= array->size) {
		resizeArray(array);
	}

	*(array->pointer + array->counter) = number;  // Pointer arithmetic
	array->counter += 1;

	return 0;
}

int getElement(struct ArrayData *array, int index) {
	if (array->counter >= array->size) {
		return -1;
	}

	int *data = array->pointer + index;
	return *data;
}
