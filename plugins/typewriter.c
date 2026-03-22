// plugins/typewriter.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include "plugin_sdk.h"
#include "plugin_common.h"

/**
 * Typewriter transformation - simulates typewriter effect by printing each character with delay
 * @param input String to print with typewriter effect
 * @return Newly allocated copy of input
 */
const char* typewriter_transform(const char* input) {
    
    // Check if input is NULL
    if (input == NULL) {
        return NULL;
    }
    
    // Print  each character of "[typewriter] " with 100ms delay
    const char* prefix = "[typewriter] ";
    int i;
    for (i = 0; prefix[i] != '\0'; i++) {
        printf("%c", prefix[i]);
        fflush(stdout); // Ensure character is printed immediately
        usleep(100000); // 100 milliseconds
    }
    
    // Print each character of input with 100ms delay
    for (i = 0; input[i] != '\0'; i++) {
        printf("%c", input[i]);
        fflush(stdout); // Ensure character is printed immediately
        usleep(100000); // 100 milliseconds
    }
    
    // Print newline at the end
    printf("\n");
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
 * Initialize the typewriter plugin
 * @param queue_size Maximum queue capacity
 * @return NULL on success, error message on failure
 */
__attribute__((visibility("default")))
const char* plugin_init(int queue_size) {
    return common_plugin_init(typewriter_transform, "typewriter", queue_size);
}