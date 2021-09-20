/*
Header file for dynamic array
*/

#ifndef ARRAY_H_
#define ARRAY_H_

struct ArrayData *initArray();
int addElement(struct ArrayData *array, int number);
int getElement(struct ArrayData *array, int index);

struct ArrayData {
  int *pointer;
  int counter;
	int size;
};

#endif
