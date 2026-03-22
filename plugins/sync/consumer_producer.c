#include <stdlib.h>
#include <string.h>
#include "consumer_producer.h"

/**
 * Initialize a consumer-producer queue
 * @param queue Pointer to queue structure
 * @param capacity Maximum number of items
 * @return NULL on success, error message on failure
 */
const char* consumer_producer_init(consumer_producer_t* queue, int capacity) {
    if (queue == NULL) {
        return "Queue pointer is NULL";
    }

    if (capacity <= 0) {
        return "Capacity must be positive";
    }

    // Allocate Array for items
    queue->items = (char**)calloc(capacity, sizeof(char*));
    if (queue->items == NULL) {
        return "Failed to allocate memory for queue items";
    }

    // Initialize queue parameters
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->finished = 0;

    // Initialize monitors
    if (monitor_init(&queue->not_full_monitor) != 0) {
        free(queue->items);
        return "Failed to initialize not_full_monitor";
    }

    if (monitor_init(&queue->not_empty_monitor) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        return "Failed to initialize not_empty_monitor";
    }

    if (monitor_init(&queue->finished_monitor) != 0) {
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->not_full_monitor);
        free(queue->items);
        return "Failed to initialize finished_monitor";
    }

    if (pthread_mutex_init(&queue->queue_mutex, NULL) != 0) {
        monitor_destroy(&queue->not_full_monitor);
        monitor_destroy(&queue->not_empty_monitor);
        monitor_destroy(&queue->finished_monitor);
        free(queue->items); 
        return "Failed to initialize queue mutex";
    }

    return NULL; // Success
}

/**
 * Destroy a consumer-producer queue and free its resources
 * @param queue Pointer to queue structure
 */
void consumer_producer_destroy(consumer_producer_t* queue) {
     if (queue == NULL) {
        return;
    }

    // Free all items in the queue
    for (int i = 0; i < queue->capacity; i++) {
        if (queue->items[i] != NULL) {
            free(queue->items[i]);
            queue->items[i] = NULL;  // Clear for safety
        }
    }

     // Free the items array
    if (queue->items != NULL) {
        free(queue->items);
    }

    // Destroy queue mutex
    pthread_mutex_destroy(&queue->queue_mutex);
    
    // Destroy monitors
    monitor_destroy(&queue->not_full_monitor);
    monitor_destroy(&queue->not_empty_monitor);
    monitor_destroy(&queue->finished_monitor);

}

/**
 * Add an item to the queue (producer).
 * Blocks if queue is full.
 * @param queue Pointer to queue structure
 * @param item String to add (queue takes ownership)
 * @return NULL on success, error message on failure
 */
const char* consumer_producer_put(consumer_producer_t* queue, const char* item) {
    if (queue == NULL) {
        return "Queue pointer is NULL";
    }

    if (item == NULL) {
        return "Item to add is NULL";
    }

    // Lock the queue mutex
    pthread_mutex_lock(&queue->queue_mutex);

    // Wait while the queue is full
    while (queue->count == queue->capacity) {

        // Reset monitor before unlocking to ensure clean state
        monitor_reset(&queue->not_full_monitor); 
        
        // Unlock the mutex before waiting
        pthread_mutex_unlock(&queue->queue_mutex);

        // Wait for space to become available
        if (monitor_wait(&queue->not_full_monitor) != 0) {
            return "Failed to wait for not_full condition";
        }

        pthread_mutex_lock(&queue->queue_mutex);
    }

    if (queue->finished) {
        // If the queue is shutting down, do not enqueue new items
        pthread_mutex_unlock(&queue->queue_mutex);
        return "Queue is shutting down";
    }

    // Make a copy of the item string (queue takes ownership)
    char* item_copy = malloc(strlen(item) + 1);
    if (item_copy == NULL) {
        pthread_mutex_unlock(&queue->queue_mutex);
        return "Failed to allocate memory for item copy";
    }
    strcpy(item_copy, item);

    // Add item to the queue
    queue->items[queue->tail] = item_copy;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->count++;

    // Signal consumers that queue is not empty
    monitor_signal(&queue->not_empty_monitor);

    // Unlock the queue mutex
    pthread_mutex_unlock(&queue->queue_mutex);

    return NULL; // Success
}

/**
 * Remove an item from the queue (consumer) and returns it.
 * Blocks if queue is empty.
 * @param queue Pointer to queue structure
 * @return String item or NULL if queue is empty
 */
char* consumer_producer_get(consumer_producer_t* queue) {
    if (queue == NULL) {
        return NULL;
    }
    
    // Lock the queue mutex
    pthread_mutex_lock(&queue->queue_mutex);

    // Wait while the queue is empty and not finished
    while (queue->count == 0  && !queue->finished) {
        
        // Reset monitor before unlocking to ensure clean state
        monitor_reset(&queue->not_empty_monitor);

        // Unlock the mutex before waiting
        pthread_mutex_unlock(&queue->queue_mutex);

        // Wait for an item to become available
        if (monitor_wait(&queue->not_empty_monitor) != 0) {
            return NULL;
        }

        pthread_mutex_lock(&queue->queue_mutex);
    }

    if (queue->count == 0 && queue->finished) {
        // If the queue is empty and processing is finished, return NULL
        pthread_mutex_unlock(&queue->queue_mutex);
        return NULL;
    }

    // Remove item from the queue
    char* item = queue->items[queue->head];
    queue->items[queue->head] = NULL;
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;

    // Signal producers that queue is not full
    monitor_signal(&queue->not_full_monitor);

    // Unlock the queue mutex
    pthread_mutex_unlock(&queue->queue_mutex);
    
    return item;
}

/**
 * Signal that processing is finished
 * @param queue Pointer to queue structure
 */
void consumer_producer_signal_finished(consumer_producer_t* queue) {
    if (queue == NULL) {
        return;
    }
    
    // Lock the queue mutex
    pthread_mutex_lock(&queue->queue_mutex);

    // Mark as finished
    queue->finished = 1;  

    // Unlock the queue mutex
    pthread_mutex_unlock(&queue->queue_mutex);
    
    // Wake up any waiting threads
    monitor_signal(&queue->finished_monitor);
    monitor_signal(&queue->not_empty_monitor);
    monitor_signal(&queue->not_full_monitor);
}

/**
 * Wait for processing to be finished
 * @param queue Pointer to queue structure
 * @return 0 on success, -1 on timeout
 */
int consumer_producer_wait_finished(consumer_producer_t* queue) {
    if (queue == NULL) {
        return -1;
    }
    
    return monitor_wait(&queue->finished_monitor);
}
