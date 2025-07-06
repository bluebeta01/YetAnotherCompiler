#ifndef LIST_H
#define LIST_H

typedef struct
{
	void *data;
	int size;
	int capacity;
} Vector;

void vector_push_back(Vector *vector, void *data, int data_size);

#define vec_at(type, vector, index) ((type*)(vector)->data)[index]
#define vec_last(type, vector) ((type*)(vector)->data)[(vector)->size - 1]
#define vec_new(type, initial_capacity) (Vector){ .data = malloc(sizeof(type) * initial_capacity), .capacity = initial_capacity }
#define vec_push(type, vector, value) vector_push_back(vector, value, sizeof(type))
#define vec_free(vector) free((vector)->data);

#endif