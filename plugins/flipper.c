// plugins/flipper.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "plugin_sdk.h"
#include "plugin_common.h"

/**
 * Flipper transformation - reverses the order of characters in the string
 * @param input String to reverse
 * @return Newly allocated reversed string for the next plugin in the chain.
 */
const char* flipper_transform(const char* input) {
    
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
    
    // Reverse the input string into result
    int i;
    for (i = 0; i < len; i++) {
        result[i] = input[len - 1 - i];
    }
    
    // Null-terminate the result string
    result[len] = '\0';
    
    return result;
}

/**
 * Initialize the flipper plugin
 * @param queue_size Maximum queue capacity
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(flipper_transform, "flipper", queue_size);
}