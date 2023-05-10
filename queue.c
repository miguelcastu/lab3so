//OS-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "queue.h"


//To create a queue
queue* queue_init(int size)
{
	// To allocate memory
	queue * q = (queue *)malloc(sizeof(queue));
	// Create the queue values
	q -> size = size;
	q -> values = malloc(sizeof(struct operation_struct_queue) * q -> size);
	q -> num_entries = 0; // Empty at the beggi
	q -> head = 0;
	q -> tail = 0;
	return q;
}


// To Enqueue an element
int queue_put(queue *q, struct operation_struct_queue* x)
{
	// If queue is full
	if (queue_full(q) == 1)
	{
		return 1;
	}
	
	// Else (queue not full)
	q -> values[q -> tail] = *x;
	
	q -> num_entries = q -> num_entries + 1;
	// Tail to 0 if end of array
	q -> tail = (q -> tail + 1) % q -> size; 
	return 0;
}


// To Dequeue an element.
struct operation_struct_queue* queue_get(queue *q)
{
	struct operation_struct_queue *elem;
	if (queue_empty(q) == 1) // If empty
	{
		exit(-1); // Exit if queue is empty
	}
	elem = &(q -> values[q -> head]);
	q -> num_entries = q -> num_entries - 1;
	// Head to 0 if end of array
	q -> head = (q -> head + 1) % q -> size;
	return elem;
}


// To check queue state
int queue_empty(queue *q)
{
	if (q -> num_entries == 0)
	{
		return 1;
	}
	return 0;
}

//To check queue full
int queue_full(queue *q)
{
	if (q -> num_entries == q -> size)
	{
		return 1;
	}
	return 0;
}


//To destroy the queue and free the resources
int queue_destroy(queue *q)
{	
	free(q);
	return 0;
}
