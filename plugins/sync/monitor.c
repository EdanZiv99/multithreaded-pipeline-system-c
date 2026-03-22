#include "monitor.h"

/**
 * Initialize a monitor
 * @param monitor Pointer to monitor structure
 * @return 0 on success, -1 on failure
 */
int monitor_init(monitor_t* monitor) {
    if (monitor == NULL) {
        return -1; 
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&monitor->mutex, NULL) != 0) {
        return -1;
    }
    
    // Initialize condition variable
    if (pthread_cond_init(&monitor->condition, NULL) != 0) {
        pthread_mutex_destroy(&monitor->mutex); // Clean up on failure
        return -1;
    }
    
    // Initialize signaled state to 0 (not signaled)
    monitor->signaled = 0;
    
    return 0;   

}

/**
 * Destroy a monitor and free its resources
 * @param monitor Pointer to monitor structure
 */
void monitor_destroy(monitor_t* monitor) {
    if (monitor == NULL) {
        return; 
    }

    // Destroy condition variable
    pthread_cond_destroy(&monitor->condition);

    // Destroy mutex
    pthread_mutex_destroy(&monitor->mutex); 

}

/**
 * Signal a monitor (sets the monitor state)
 * @param monitor Pointer to monitor structure
 */
void monitor_signal(monitor_t* monitor) {
     if (monitor == NULL) {
        return;
    }

    // Lock the mutex
    pthread_mutex_lock(&monitor->mutex);

    // Set the signaled state to 1
    monitor->signaled = 1;

    // Signal the condition variable
    pthread_cond_signal(&monitor->condition);

    // Unlock the mutex
    pthread_mutex_unlock(&monitor->mutex);

}

/**
 * Reset a monitor (clears the monitor state)
 * @param monitor Pointer to monitor structure
 */
void monitor_reset(monitor_t* monitor) {
    if (monitor == NULL) {
        return;
    }

    // Lock the mutex
    pthread_mutex_lock(&monitor->mutex);

    // Reset the signaled state to 0
    monitor->signaled = 0;

    // Unlock the mutex
    pthread_mutex_unlock(&monitor->mutex);

}

/**
 * Wait for a monitor to be signaled (infinite wait)
 * @param monitor Pointer to monitor structure
 * @return 0 on success, -1 on error
 */
int monitor_wait(monitor_t* monitor) {
    if (monitor == NULL) {
        return -1;
    }

    // Lock the mutex
    pthread_mutex_lock(&monitor->mutex);

    // Wait until the monitor is signaled
    while (monitor->signaled == 0) {
        if (pthread_cond_wait(&monitor->condition, &monitor->mutex) != 0) {
            // If waiting fails, unlock the mutex and return error
            pthread_mutex_unlock(&monitor->mutex);
            return -1;
        }
    }
    
    // Unlock the mutex
    pthread_mutex_unlock(&monitor->mutex);

    return 0;

}
