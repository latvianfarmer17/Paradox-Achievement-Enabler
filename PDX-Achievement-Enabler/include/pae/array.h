#ifndef PAE_ARRAY_H
#define PAE_ARRAY_H

#include <malloc.h>

#define ARRAY_DEFAULT_CAPACITY 32

typedef long long unsigned int array_t;

typedef struct _array {
	array_t* data;
	int size;
	int capacity;
} array;

int array_init(array* arr);
int array_clear(array* arr, int reset_capacity);
int array_free(array* arr);
int array_push_back(array* arr, array_t element);

#endif