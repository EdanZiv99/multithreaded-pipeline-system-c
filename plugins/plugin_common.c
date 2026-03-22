#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "plugin_common.h"

// Global plugin context shared by all functions in this module
static plugin_context_t g_context;

/**
 * Generic consumer thread function
 * This function runs in a separate thread and processes items from the queue
 * @param arg Pointer to plugin_context_t
 * @return NULL
 */
void* plugin_consumer_thread(void* arg)
{
    plugin_context_t* context = (plugin_context_t*)arg;

    while (1) {
        // Get the next item from the queue
        char* item = consumer_producer_get(context->queue);

        // NULL means queue is finished and empty
        if (item == NULL) {
            break;
        }

        // Handle <END> signal
        if (strcmp(item, "<END>") == 0) {
            // Forward <END> to next plugin if attached
            if (context->next_place_work != NULL) {
                const char* error = context->next_place_work("<END>");
                if (error != NULL) {
                    log_error(context, error);
                }
            }

            // Free the <END> message
            free(item);

            // Mark as finished before signaling
            context->finished = 1;

            // Signal that this queue is finished
            consumer_producer_signal_finished(context->queue);
            break;
        }

        // Process the item using the plugin's processing function
        const char* processed = context->process_function(item);

        // Forward the result to the next plugin if attached
        if (context->next_place_work != NULL) {
            const char* error = context->next_place_work(processed);
            if (error != NULL) {
                log_error(context, error);
            }
            if (processed != NULL) {
                free((void*)processed);
            }
        } else {
            // If this is the last plugin, free the processed string
            if (processed != NULL) {
                free((void*)processed);
            }
        }

        // Free the original item after processing 
        free(item);
    }

    return NULL;
}

/**
 * Print error message in the format [ERROR][Plugin Name] - message
 * @param context Plugin context
 * @param message Error message
 */
void log_error(plugin_context_t* context, const char* message)
{
     if (context != NULL && context->name != NULL && message != NULL) {
        fprintf(stderr, "[ERROR][%s] - %s\n", context->name, message);
    }
}

/**
 * Print info message in the format [INFO][Plugin Name] - message
 * @param context Plugin context
 * @param message Info message
 */
void log_info(plugin_context_t* context, const char* message)
{
    if (context != NULL && context->name != NULL && message != NULL) {
        fprintf(stdout, "[INFO][%s] - %s\n", context->name, message);
    }
}

/**
 * Get the plugin's name
 * @return The plugin's name (should not be modified or freed)
 */
__attribute__((visibility("default")))
const char* plugin_get_name(void)
{
    return g_context.name;
}

/**
 * Initialize the common plugin infrastructure with the specified queue size
 * @param process_function Plugin-specific processing function
 * @param name Plugin name
 * @param queue_size Maximum number of items that can be queued
 * @return NULL on success, error message on failure
 */
const char* common_plugin_init(const char* (*process_function)(const char*), const char* name, int queue_size)
{
    // Prevent multiple initializations
    if (g_context.initialized) {
        return "Plugin already initialized";
    }

    // Validate parameters
    if (process_function == NULL) {
        return "Process function is NULL";
    }

    if (name == NULL) {
        return "Plugin name is NULL";
    }

    if (queue_size <= 0) {
        return "Queue size must be positive";
    }

    // Initialize the plugin context
    g_context.name = name;
    g_context.process_function = process_function;
    g_context.finished = 0; 
    g_context.next_place_work = NULL;  

    // Allocte memory for the queue
    g_context.queue = (consumer_producer_t*)malloc(sizeof(consumer_producer_t));
    if (g_context.queue == NULL) {
        return "Failed to allocate memory for queue";
    }

    // Initialize the queue with the specified size
    const char* error = consumer_producer_init(g_context.queue, queue_size);
    if (error != NULL) {
        free(g_context.queue);
        g_context.queue = NULL;
        return error;
    }

    // Initialize the consumer thread with the plugin context
    int result = pthread_create(&g_context.consumer_thread, NULL, plugin_consumer_thread, &g_context);
    if (result != 0) {
        consumer_producer_destroy(g_context.queue);
        free(g_context.queue);
        g_context.queue = NULL;
        return "Failed to create consumer thread";
    }

    // Mark plugin as successfully initialized
    g_context.initialized = 1;

    return NULL; // Success
}

/**
 * Finalize the plugin - drain queue and terminate thread gracefully (i.e. pthread_join)
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_fini(void)
{
    // Check if plugin was initialized
    if (!g_context.initialized) {
        return "Plugin not initialized";
    }

    // Wait for the consumer thread to finish processing 
     if (pthread_join(g_context.consumer_thread, NULL) != 0) {
        return "Failed to join consumer thread";
    }

    // Destroy the producer_consumer queue and free memory
    if (g_context.queue != NULL) {
        consumer_producer_destroy(g_context.queue);
        free(g_context.queue);
        g_context.queue = NULL;
    }

     // Reset plugin context fields to initial state
    g_context.name = NULL;
    g_context.consumer_thread = 0;
    g_context.next_place_work = NULL;
    g_context.process_function = NULL;
    g_context.initialized = 0;
    g_context.finished = 0;

    return NULL; // Success

}

/**
 * Place work (a string) into the plugin's queue
 * @param str The string to process (plugin takes ownership if it allocates new memory)
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_place_work(const char* str)
{
    // Check if plugin was initialized
    if (!g_context.initialized) {
        return "Plugin not initialized";
    }

    // Check if plugin has already finished processing
    if (g_context.finished) {
        return "Plugin has finished processing";
    }

    // Check if str is NULL
    if (str == NULL) {
        return "Input string is NULL";
    }

    // Add item to queue
    return consumer_producer_put(g_context.queue, str);
}

/**
 * Attach this plugin to the next plugin in the chain
 * @param next_place_work Function pointer to the next plugin's place_work function
 */
__attribute__((visibility("default")))
void plugin_attach(const char* (*next_place_work)(const char*))
{
    g_context.next_place_work = next_place_work;
}

/**
 * Wait until the plugin has finished processing all work and is ready to shutdown
 * This is a blocking function used for graceful shutdown coordination
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_wait_finished(void)
{
    // Check if plugin was initialized
    if (!g_context.initialized) {
        return "Plugin not initialized";
    }

    // Wait until the queue is finished processing
    int result = consumer_producer_wait_finished(g_context.queue);
    if (result != 0) {
        return "Failed to wait for queue to finish";
    }

    return NULL; // Success
}