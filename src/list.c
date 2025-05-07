#include <stdlib.h>
#include <string.h>
#include "list.h"

void vector_push_back(Vector *vector, void *data, int data_size)
{
	if (vector->size == vector->capacity)
	{
		int new_capacity = vector->capacity * 2 + 1;
		char *new_buffer = malloc(data_size * new_capacity);
		memcpy(new_buffer, vector->data, data_size * vector->size);
		free(vector->data);
		vector->data = (void*)new_buffer;
		vector->capacity = new_capacity;
	}

	memcpy((char*)vector->data + data_size * vector->size, data, data_size);

	vector->size++;
}