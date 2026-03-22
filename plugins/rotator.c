// plugins/rotator.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_sdk.h"
#include "plugin_common.h"

/**
 * Rotator transformation - moves every character one position to the right
 * The last character wraps around to become the first character
 * @param input String to rotate
 * @return Newly allocated rotated string
 */
const char* rotator_transform(const char* input) {
    
    // Check if input is NULL
    if (input == NULL) {
        return NULL;
    }
    
    // Get the length of the input string
    int len = strlen(input);
    
    // Allocate memory for the result string
    char* result = malloc(len + 1);
    if (result == NULL) {
        return NULL;
    }
    
    // Handle empty string or single character
    if (len <= 1) {
        strcpy(result, input);
        return result;
    }
    
    // Move last character to the front
    result[0] = input[len - 1];
    
    // Shift all other characters one position to the right
    int i;
    for (i = 1; i < len; i++) {
        result[i] = input[i - 1];
    }
    
    // Null-terminate the result string
    result[len] = '\0';
    
    return result;
}

/**
 * Initialize the rotator plugin
 * @param queue_size Maximum queue capacity 
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(rotator_transform, "rotator", queue_size);
}