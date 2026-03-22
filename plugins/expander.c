// plugins/expander.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_sdk.h"
#include "plugin_common.h"

/**
 * Expander transformation - inserts a single space between each character
 * @param input String to expand
 * @return Newly allocated expanded string
 */
const char* expander_transform(const char* input) {
    
    // Check if input is NULL
    if (input == NULL) {
        return NULL;
    }
    
    // Get the length of the input string
    int len = strlen(input);
    
    // Handle empty string
    if (len == 0) {
        char* result = malloc(1);
        if (result == NULL) {
            return NULL;
        }
        result[0] = '\0';
        return result;
    }
    
    // Calculate size needed for the new string
    int new_len = len + (len - 1);
    
    // Allocate memory for the expanded string
    char* result = malloc(new_len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    // Build the expanded string
    int j = 0;
    int i;
    for (i = 0; i < len; i++) {
        // Add the character
        result[j++] = input[i];
        
        // Add space after each character except the last one
        if (i < len - 1) {
            result[j++] = ' ';
        }
    }
    
    // Null-terminate the result string
    result[j] = '\0';
    
    return result;
}

/**
 * Initialize the expander plugin
 * @param queue_size Maximum queue capacity
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(expander_transform, "expander", queue_size);
}