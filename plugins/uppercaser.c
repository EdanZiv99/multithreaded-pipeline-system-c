// plugins/uppercaser.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h> 
#include "plugin_sdk.h"
#include "plugin_common.h"

/**
 * Uppercaser transformation - converts all lowercase letters to uppercase
 * @param input String to convert to uppercase
 * @return Newly allocated uppercase string for the next plugin in the chain.
 */
const char* uppercaser_transform(const char* input) {
    
    // Check if input is NULL
    if (input == NULL) {
        return NULL;
    }
    
    // Allocate memory for the result string
    char* result = malloc(strlen(input) + 1);
    if (result == NULL) {
        return NULL;
    }
    
    // Convert each character to uppercase
    int i;
    for (i = 0; input[i] != '\0'; i++) {
        result[i] = toupper((unsigned char)input[i]);
    }
    
    // Null-terminate the result string
    result[i] = '\0';
    
    return result;
}

/**
 * Initialize the uppercaser plugin
 * @param queue_size Maximum queue capacity
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(uppercaser_transform, "uppercaser", queue_size);
} 