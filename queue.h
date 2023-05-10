#ifndef HEADER_FILE
#define HEADER_FILE


 struct operation_struct_queue{
    int op_number;
    char type[10];
    int arg1;
    int arg2;
    int arg3;
} ;

typedef struct queue
{
	// Define the struct yourself
	struct operation_struct_queue *values;
	int head, tail, num_entries, size;
}queue;

queue* queue_init (int size);
int queue_destroy (queue *q);
int queue_put (queue *q, struct operation_struct_queue* received_operation);
struct operation_struct_queue * queue_get(queue *q);
int queue_empty (queue *q);
int queue_full(queue *q);

#endif
