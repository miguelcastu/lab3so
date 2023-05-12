#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include "queue.h"
#include <string.h>
#include <sys/types.h>
#include <math.h>


/**
 * Entry point
 * @param argc
 * @param argv
 * @return
*/

// Circular buffer definition
struct queue *c_buffer;

// Path to the entered file (constant character)
const char *path_file;

// Mutexes definition
pthread_mutex_t mutex; // Mutex to access shared buffer
pthread_mutex_t mutex_file; // Mutex to access files

// Conditional Variables definition
pthread_cond_t non_full; // Controls element additions
pthread_cond_t non_empty; // Controls element removal

// Global variable where the result is stored defition
int result = 0;

// Parameters definition
struct parameters{int number_parameter; int id_parameter;};


// Producer code
// Similar to the one seen in class slides, plus error handling methods
void *cproducer(void *arg)
{
    // Variable Declarations
    int line = 0, p_id, p_type, p_time = 0;
    char ending_char;

    // Producer parameters obtention
    struct parameters *producer_parameters = arg;

    // Open file given path_file - read only - pointer
    FILE *file = fopen(path_file, "r");

    // Lock mutex_file
    if (pthread_mutex_lock(&mutex_file) < 0)
    {   
        // If error when locking mutex_file
        perror("ERROR: mutex_file lock");
        exit(-1);
    }

    // If error when opening path_file
    if (file == NULL)
    {
        perror("ERROR: fopen path_file");
        exit(-1);
    }

    // File lines counter
    while (producer_parameters -> id_parameter > line)
    {
        ending_char = fgetc(file);
        if (ending_char == '\n')
        {
            // If next line, lines number + 1
            line++;
        }
    }

    // Unlock mutex_file
    if (pthread_mutex_unlock(&mutex_file) < 0)
    {
        // If error when unlocking mutex_file
        perror("ERROR: mutex_file unlock");
        exit(-1);
    }

    // Values for storage in the global arrays - for all the producers
    for (int i = 0; producer_parameters -> number_parameter > i; i++)
    {
        // Read formatted file
        if (fscanf(file, "%d %d %d", &p_id, &p_type, &p_time) != 3)
        {
            // If not received the 3 arguments
            perror("ERROR: fscanf p_id, p_type, p_time");
            exit(-1);
        }

        // Store p_type and p_time
        struct element producer_data = {.type = p_type, .time = p_time};

        //  Lock mutex shared buffer
        if (pthread_mutex_lock(&mutex) < 0)
        {
            // If error when locking mutex shared buffer
            perror("ERROR: mutex lock");
            exit(-1);
        }
        
        // Access buffer and wait (blocked) while buffer full
        while(queue_full(c_buffer) == 1)
        {
            // Thread non_full suspeded, releases mutex (thread wait)
            if (pthread_cond_wait(&non_full, &mutex) < 0)
            {
                // If error when thread non_full suspended
                perror("ERROR: conditional variable non_full wait");
                exit(-1);
            }
        }
        
        // Enqueue type_time_data in circular buffer
        if (queue_put(c_buffer, &producer_data) == 1)
        {
            // If error when enqueuing type_time_data in circular buffer
            perror("ERROR: enqueue in c_buffer type_time_data");
            exit(-1);
        }
        
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
    }
    // Exit thread
    pthread_exit(0);
}


// Consumer code
// Similar to the one seen in class slides, plus error handling methods
void *cconsumer(int *data_size)
{
    // While (costant - infinite) until break
    while(1)
    {
        //  Lock mutex shared buffer
        if (pthread_mutex_lock(&mutex) < 0)
        {   
            // If error when locking mutex shared buffer
            perror("ERROR: mutex lock");
            exit(-1);
        }
        
        // If no data left - mutex shared buffer unlock
        if (*data_size < 1)
        {
            pthread_mutex_unlock(&mutex_buffer);
            pthread_exit(0);
        }
        
        // Else, if there exist data left, continue
        *data_size = *data_size - 1;

        // Access buffer and wait (blocked) while buffer empty
        while(queue_empty(c_buffer) == 1)
        {
            // Thread non_empty suspeded, releases mutex (thread wait)
            if (pthread_cond_wait(&non_empty, &mutex) < 0)
            {
                // If error when thread non_empty suspended
                perror("ERROR: conditional variable non_empty wait");
                exit(-1);
            }
        }

        // Consumer data enqueue
        
        struct element *consumer_data = queue_get(c_buffer);

        // If error with time format
        if (consumer_data == NULL)
        {
            perror("ERROR: wrong time format");
            exit(-1);
        }

        switch (consumer_data -> type)
        {
            // Common node - price: 3
            case 1:
                result += consumer_data -> time * 3;
                break;

            // Computation node - price: 6
            case 2:
                result += consumer_data->time * 6;
                break;

            // Super-computer - price: 15
            case 3:
                result += consumer_data->time * 15;
                break;

            // If error with type format
            default:
                perror("ERROR: wrong type format");
        }

        // Ending *cconsumer
        // Unblock thread non_full through signal 
        if (pthread_cond_signal(&non_full) < 0)
        {   
            // If error when unblocking thread non_full
            perror("ERROR: conditional variable signal non_full");
            exit(-1);
        }

        // Unlock mutex shared buffer
        if (pthread_mutex_unlock(&mutex) < 0)
        {
            // If error when unlocking mutex shared buffer
            perror("ERROR: mutex unlock");
            exit(-1);
        }
    }
    // Exit thread
    pthread_exit(0);
}


// Main process
int main (int argc, const char * argv[])
{
    path_file = malloc(sizeof(char[strlen(argv[1])]));  // Allocate memory for the file name
    path_file = argv[1];    // Copy file name to a global variable for later use by producer threads
    int producers_number = atoi(argv[2]);
    int consumers_number = atoi(argv[3]); // Number of producers Number of consumers
    int queue_size = atoi(argv[4]);
    int operations, line = 0, id_operation = 1, i = 0; // Number of operations indicated at the beginning of the file
    char ending_char;

    // If error with the ammount of arguments provided (5)
    if (argc > 5)
    {
        perror("Error: Invalid number of arguments");
        return -1;
    }

    // Open m_file (main file) given argv[1] - read only - pointer
    FILE *m_file = fopen(argv[1], "r");
    
    // If error when opening argv[1]
    if (m_file == NULL)
    {
        perror("ERROR: fopen m_file");
        return -1;
    }

    // While not end of file, m_file lines counter
    while (!feof(m_file))
    {
        ending_char = fgetc(m_file);
        if (ending_char == '\n')
        {
            // If next line, lines number + 1
            line++;
        }
    
    // If error when more producers than operations
    if (producers_number > operations)
    {
        perror("ERROR: more producers than operations");
        return -1;
    }


    // MAIN INITIALIZATION
    // Circular buffer initialization with its queue size
    c_buffer = queue_init(queue_size);

    // Mutexes initialization
    // Mutex shared buffer initialization
    if (pthread_mutex_init(&mutex, NULL) < 0)
    {
        // If error when initializing mutex
        perror("ERROR: mutex initialization");
        return -1;
    }

    // Mutex to access file initialization
    if (pthread_mutex_init(&mutex_file, NULL) < 0)
    {
        // If error when initializing mutex_file
        perror("ERROR: mutex_file initialization");
        return -1;
    }

    // Conditional Variables initialization
    // non_full conditional variable initialization
    if (pthread_cond_init(&non_full, NULL) < 0)
    {
        // If error when initializing non_full conditional variable
        perror("ERROR: conditional variable non_full initialization");
        return -1;
    }

    // non_empty conditional variable initialization
    if (pthread_cond_init(&non_empty, NULL) < 0)
    {
        // If error when initializing non_full conditional variable
        perror("ERROR: conditional variable non_empty initialization");
        return -1;
    }

    
    // Operations per producer
    int producers_operations = floor(operations / producers_number);

    pthread_t producers_threads[producers_number]; // Producer threads
    pthread_t consumers_threads[consumers_number]; // Consumer threads

    // Parmeters for producers
    struct parameters producer_parameter[producers_number];

    // THREAD CREATION
    // Producer threads creation
    for (i; producers_number - 1 > i; i++)
    {   
        // Start at id_operation
        producer_parameter[i].id_parameter = id_operation; 
        
        // Operations number of the producer
        producer_parameter[i].number_parameter = producers_operations;

        // Producer threads creation
        if (pthread_create(&producers_threads[i], NULL, (void *)cproducer, &producer_parameter[i]) < 0)
        {
            // If error when creating producer threads
            perror("ERROR: creating producers_threads");
            exit(-1);
        }

        // Next operation id (operation id + producer operations)
        id_operation += producers_operations; 
    }
    
    // Last producer
    // Last producer operations
    int id_operation_last = operations - i * producers_operations;

    // Start at id_operation (last producer)
    producer_parameter[producers_number - 1].id_parameter = id_operation;

    // Operations number of the last producer
    producer_parameter[producers_number - 1].number_parameter = id_operation_last;

    // Last producer thread creation
    if (pthread_create(&producers_threads[producers_number - 1], NULL, (void *)cproducer, &producer_parameter[producers_number - 1]) < 0)
    {
        // If error when creating last producer thread
        perror("ERROR: creating last producers_threads");
        exit(-1);
    }

    // Consumer threads creation
    // Number of operations for the consumers
    int operations_counter = operations;
    
    // Consumer threads creation
    for (int i = 0; consumers_number > i; i++)
    {   
        // Consumer threads creation
        if (pthread_create(&consumers_threads[i], NULL, (void *)workers, NULL) < 0)
        {
            // If error when creating consumer threads
            perror("ERROR: creating consumers_threads");
            return -1;
        }
    }

    // MAIN - PROCESS TERMINATION
    // Wait for thread termination
    // Producers threads join - wait termination of producers threads
    for (int i = 0; producers_number > i; i++)
    {
        // Producers threads join
        if (pthread_join(producers_threads[i], NULL) < 0)
        {
            // If error when joining producers threads
            perror("ERROR: joining producers_threads");
            return -1;
        }
    }

    // Consumers threads join - wait termination of consumers threads
    for (int i = 0; consumers_number > i; i++)
    {
        // Consumers threads join
        if (pthread_join(consumers_threads[i], NULL) < 0)
        {
            // If error when joining consumers threads
            perror("ERROR: joining consumers_threads");
            return -1;
        }
    }

    // Close m_file
    if (fclose(m_file) < 0)
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

    // Destroy mutex_file mutex
    if (pthread_mutex_destroy(&mutex_file) < 0)
    {
        // If error when destroying mutex_file mutex
        perror("ERROR: destroying mutex_file");
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

    // Print the result of the execution
    printf("Total: %d euros.\n", result);

    // Finished, return 0
    return 0;
}
