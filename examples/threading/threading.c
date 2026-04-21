#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

// Optional: use these functions to add debug or error prints to your application
//#define DEBUG_LOG(msg,...)
#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{
    // TODO: wait, obtain mutex, wait, release mutex as described by thread_data structure
    // hint: use a cast like the one below to obtain thread arguments from your parameter
    //struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    struct thread_data* thread_func_args = (struct thread_data *) thread_param;
    
    // Wait
    DEBUG_LOG("Waiting to obtain the mutex...");
    usleep(thread_func_args->wait_to_obtain_ms * 1000);
    
    // Obtain mutex
    DEBUG_LOG("Trying to lock the mutex...");
    if (pthread_mutex_lock(thread_func_args->mutex) != 0)
    {
    	ERROR_LOG("Mutex could not be locked");
    	
    	thread_func_args->thread_complete_success = false;
    	return thread_param;
    }
    
    // Wait
    DEBUG_LOG("Waiting to release the mutex...");
    usleep(thread_func_args->wait_to_release_ms * 1000);
    
    // Release mutex
    DEBUG_LOG("Trying to unlock the mutex...");
    if (pthread_mutex_unlock(thread_func_args->mutex) != 0)
    {
    	ERROR_LOG("Mutex could not be unlocked");
    	
    	thread_func_args->thread_complete_success = false;
    	return thread_param;
    }
    
    DEBUG_LOG("Successfully ending threadfunc...");
    thread_func_args->thread_complete_success = true;
    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex, int wait_to_obtain_ms, int wait_to_release_ms)
{
    /**
     * TODO: allocate memory for thread_data, setup mutex and wait arguments, pass thread_data to created thread
     * using threadfunc() as entry point.
     *
     * return true if successful.
     *
     * See implementation details in threading.h file comment block
     */
     
     // Allocate memory for thread_param
     DEBUG_LOG("Allocating dynamic memory for thread_data...");
     struct thread_data *thread_param;
     thread_param = (struct thread_data *)malloc(sizeof(struct thread_data));
     if (thread_param == NULL)
     {
     	ERROR_LOG("Dynamic memory allocation failed");
     	return false;
     }
     
     // Setup mutex and wait arguments
     DEBUG_LOG("Setting up thread_data members...");
     thread_param -> mutex = mutex;    
     thread_param -> wait_to_obtain_ms = wait_to_obtain_ms;
     thread_param -> wait_to_release_ms = wait_to_release_ms;
     
     // Create thread
     DEBUG_LOG("Creating the thread...");
     if (pthread_create(thread, NULL, threadfunc, thread_param) != 0)
     {
     	ERROR_LOG("Thread could not be created");
     	
     	// Free allocated memory for thread_param
     	free(thread_param);
     	
     	return false;
     }
     
     return true;
}

