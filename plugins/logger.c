// plugins/logger.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_sdk.h"
#include "plugin_common.h"

/**
 * Logger transformation - prints string with "[logger]" prefix and passes through unchanged
 * @param input String to log
 * @return Newly allocated copy for the next plugin in the chain.
 */
const char* logger_transform(const char* input) {
    
    // Check if input is NULL
    if (input == NULL) {
        return NULL;
    }
    
    // Print the input string with "[logger]" prefix
    printf("[logger] %s\n", input);
    fflush(stdout);
    
    // Allocate memory for the result and copy the input string
    char* result = malloc(strlen(input) + 1);
    if (result == NULL) {
        return NULL;
    }
    strcpy(result, input);
    return result;
}

/**
 * Initialize the logger plugin
 * @param queue_size Maximum queue capacity 
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(logger_transform, "logger", queue_size);
}