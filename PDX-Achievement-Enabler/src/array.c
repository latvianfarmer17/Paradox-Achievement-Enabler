#include <pae/array.h>

int array_init(array* arr) {
	arr->data = (array_t*)malloc(ARRAY_DEFAULT_CAPACITY * sizeof(array_t));
	arr->size = 0;
	arr->capacity = ARRAY_DEFAULT_CAPACITY;

	return arr->data == NULL;
}

int array_clear(array* arr, int reset_capacity) {
	array_free(arr);

	arr->size = 0;
	arr->capacity = reset_capacity ? ARRAY_DEFAULT_CAPACITY : arr->capacity;
	arr->data = (array_t*)malloc(arr->capacity * sizeof(array_t));

	return arr->data == NULL;
}

int array_free(array* arr) {
	if (arr->data == NULL) {
		return 1;
	}

	free(arr->data);

	return 0;
}

int array_push_back(array* arr, array_t element) {
	if (arr->data == NULL) {
		return 1;
	}

	while (arr->size >= arr->capacity) {
		arr->capacity <<= 1;
	}

	array_t* tmp = realloc(arr->data, arr->capacity * sizeof(array_t));

	if (tmp == NULL) {
		return 1;
	}

	arr->data = tmp;
	arr->data[arr->size++] = element;

	return 0;
}