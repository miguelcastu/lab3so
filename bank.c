    //OS-P3 2022-2023

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/stat.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

struct queue *c_buffer;

// Path to the entered file (constant character)
const char *file_name;

// Mutexes definition
pthread_mutex_t mutex; // Mutex to access shared buffer

// Conditional Variables definition
pthread_cond_t non_full; // Controls element additions
pthread_cond_t non_empty; // Controls element removal


int client_numop = 0;
int bank_numop = 0;
int global_balance = 0;

struct operation_struct{
    int op_number;
    char type[10];
    int arg1;
    int arg2;
    int arg3;
} ;

void *atm(void *operation_received)
{
    struct operation_struct * single_operation = (struct operation_struct*) operation_received;

    // Values for storage in the global arrays - for all the producers 
    //  Lock mutex shared buffer
    if (pthread_mutex_lock(&mutex) < 0)
    {
        // If error when locking mutex shared buffer
        perror("ERROR: mutex lock");
        exit(-1);
    }
   
    
    
    // Enqueue type_time_data in circular buffer
    if (queue_put(c_buffer, operation_received) == 1)
    {
        // If error when enqueuing type_time_data in circular buffer
        perror("ERROR: enqueue in c_buffer type_time_data");
        exit(-1);
    }
    client_numop +=1;

    // Unblock thread non_empty through signal 
    if (pthread_cond_signal(&non_empty) < 0)
    {
        // If error when unblocking thread non_empty
        perror("ERROR: conditional variable signal non_empty");
        exit(-1);
    }
    
    // Unlock mutex shared buffer
    if (pthread_mutex_unlock(&mutex) < 0)
    {
        // If error when unlocking mutex shared buffer
        perror("ERROR: mutex unlock");
        exit(-1);
    }
    // Exit thread
    pthread_exit(0);
}


/**
 * Entry point
 * @param argc
 * @param argv
 * @return
 */
int main (int argc, const char * argv[] ) {
    
    file_name = argv[1];
    int num_atms = atoi(argv[2]);
    int num_workers = atoi(argv[3]); 
    //no se para que se usa la variable max_accounts
    int max_accounts = atoi(argv[4]);
    int buff_size = atoi(argv[5]);
    int max_operations, linea = 0, i = 0; 
    char ending_char;

    
    if (argc > 6)
    {
        perror("Error: Invalid number of arguments");
        return -1;
    }

    FILE *o_file = fopen(argv[1], "r");
    
    // If error when opening argv[1]
    if (o_file == NULL)
    {
        perror("ERROR: when openning o_file");
        return -1;
    }

    // Obtain max number of operations from file
    if (fscanf(o_file, "%d", &max_operations) < 0)
    {
        // If not received operations
        perror("ERROR: fscanf operations");
        return -1;
    }

    // While not end of file, m_file lines counter
    while (!feof(o_file))
    {
        ending_char = fgetc(o_file);
        if (ending_char == '\n')
        {
            // If next line, lines number + 1
            linea++;
        }
    }

    // Exceed the maximum number of 200 operations=ERROR
    if (max_operations > 200)
    {
        perror("ERROR: wrong operations number. More than 200");
        return -1;
    }

    // Less lines than max_operations = ERROR
    if ((max_operations+1) > linea)//Taking into account the max operations line
    {
        perror("ERROR: wrong operations number ");
        return -1;
    }

    if (num_atms <= 0 )
    {
        perror("ERROR: wrong atms number");
        return -1;
    }

    if (num_workers <= 0 )
    {
        perror("ERROR: wrong workers number");
        return -1;
    }

    // If error when more producers than operations
    if (num_atms > max_operations)
    {
        perror("ERROR: more producers than operations");
        return -1;
    }

    // If error when buffer size is smaller than 0
    if (buff_size <= 0)
    {
        perror("ERROR: queue size must be greater than 0");
        return -1;
    }
    //code to save in the list_clients_ops all the operations of the file
    rewind(o_file);
    char *line = NULL;
    size_t len = 0;
    int count = 0;
    ssize_t read;

    // Allocate memory for the list of operations
    int max_operations_plus_one = max_operations + 1; // Taking into account the max operations line
    struct operation_struct *list_clients_ops = malloc(max_operations_plus_one * sizeof(struct operation_struct));
    if (list_clients_ops == NULL) {
        perror("ERROR: couldn't allocate memory");
        return -1;
    }
    int first_line=1;
    while ((read = getline(&line, &len, o_file)) != -1) {
        if (count >= max_operations) {
            break; // Stop reading the file if we have reached the max number of operations
        }
        if (first_line!=1){//saltamos la primera linea
        sscanf(line, "%s %d %d %d \n", list_clients_ops[count].type, &list_clients_ops[count].arg1, &list_clients_ops[count].arg2, &list_clients_ops[count].arg3);
        list_clients_ops[count].op_number = count + 1;
        printf("%d: %s %d %d %d",list_clients_ops[count].op_number,list_clients_ops[count].type,list_clients_ops[count].arg1, list_clients_ops[count].arg2, list_clients_ops[count].arg3);
        count++;
        }
        first_line=0;
    }
    // Free memory used by line variable
    free(line);
    line = NULL;
    len = 0;

    // Check if we read the expected number of operations
    if (count < max_operations) {
        perror("ERROR: wrong operations number");
        return -1;
    }


    // Circular buffer initialization with its queue size
    c_buffer = queue_init(buff_size);

    // Mutexes initialization
    if (pthread_mutex_init(&mutex, NULL) < 0)
    {
        perror("ERROR: mutex initialization");
        return -1;
    }

    // Conditional Variables initialization
    if (pthread_cond_init(&non_full, NULL) < 0)
    {
        perror("ERROR: conditional variable non_full initialization");
        return -1;
    }

    if (pthread_cond_init(&non_empty, NULL) < 0)
    {
        perror("ERROR: conditional variable non_empty initialization");
        return -1;
    }

    pthread_t atms_threads[num_atms]; // Producer threads
    pthread_t worker_threads[num_workers]; // Consumer threads
    // THREAD CREATION of ATMS
    for (i; i<num_atms-1; i++)
    {   

        // Producer threads creation
        if (pthread_create(&atms_threads[i], NULL, atm, &list_clients_ops[i]) < 0)
        {
            // If error when creating producer threads
            perror("ERROR: creating producers_threads");
            exit(-1);
        }

        // Next operation id (operation id + producer operations)
    }
    free(list_clients_ops);

    // Close m_file
    if (fclose(o_file) < 0)
    {
        // If error when closing m_file
        perror("ERROR: closing m_file");
        return -1;
    }

    // DESTRUCTION
    // Destroy Mutexes
    // Destroy mutex shared buffer mutex
    if (pthread_mutex_destroy(&mutex) < 0)
    {
        // If error when destroying mutex mutex
        perror("ERROR: destroying mutex");
        return -1;
    }


    // Destroy Conditional Variables
    // Destroy non_empty Conditional Variable
    if (pthread_cond_destroy(&non_empty) < 0)
    {
        // If error when destroying non_empty conditional variable
        perror("ERROR: destroying conditional variable non_empty");
        return -1;
    }

    // Destroy non_full Conditional Variable
    if (pthread_cond_destroy(&non_full) < 0)
    {
        // If error when destroying non_full conditional variable
        perror("ERROR: destroying conditional variable non_full");
        return -1;
    }

    // Destroy queue (buffer)
    queue_destroy(c_buffer);

    return 0;
}