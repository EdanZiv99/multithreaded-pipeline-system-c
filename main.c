// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include "plugins/plugin_sdk.h"

#define MAX_LINE_LEN 1026

// Structure to hold all plugin function pointers and metadata
typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    plugin_get_name_func_t get_name; 
    char* name;
    void* handle;
} plugin_handle_t;

/**
 * Try to open plugin from multiple possible locations
 * @param plugin_name Name of the plugin without .so extension
 * @param out_path Buffer to store the successful path
 * @param out_sz Size of the output buffer
 * @return Handle to the loaded library or NULL if not found
 */
static void* try_open_plugin(const char* plugin_name, char* out_path, size_t out_sz) {
    void* handle;

    // Try current directory 
    snprintf(out_path, out_sz, "./%s.so", plugin_name);
    handle = dlopen(out_path, RTLD_NOW | RTLD_LOCAL);
    if (handle) {
        return handle;
    }

    // Try output directory
    snprintf(out_path, out_sz, "./output/%s.so", plugin_name);
    handle = dlopen(out_path, RTLD_NOW | RTLD_LOCAL);
    if (handle) {
        return handle;
    }

    // Try output directory with relative path
    snprintf(out_path, out_sz, "output/%s.so", plugin_name);
    return dlopen(out_path, RTLD_NOW | RTLD_LOCAL);
}

/**
 * Print usage instructions to stdout
 * Shows available plugins and example usage
 */
void print_usage(void) {
    printf("Usage: ./analyzer <queue_size> <plugin1> <plugin2> ... <pluginN>\n");
    printf("Arguments:\n");
    printf("  queue_size   Maximum number of items in each plugin's queue\n");
    printf("  plugin1..N   Names of plugins to load (without .so extension)\n");
    printf("\n");
    printf("Available plugins:\n");
    printf("  logger       - Logs all strings that pass through\n");
    printf("  typewriter   - Simulates typewriter effect with delays\n");
    printf("  uppercaser   - Converts strings to uppercase\n");
    printf("  rotator      - Move every character to the right. Last character moves to the beginning.\n");
    printf("  flipper      - Reverses the order of characters\n");
    printf("  expander     - Expands each character with spaces\n");
    printf("\n");
    printf("Example:\n");
    printf("  ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo 'hello' | ./analyzer 20 uppercaser rotator logger\n");
    printf("  echo '<END>' | ./analyzer 20 uppercaser rotator logger\n");
}

/**
 * Clean up all loaded plugins
 * Used for error cleanup, does not call fini() 
 * @param plugins Array of plugin handles
 * @param count Number of plugins to clean up
 */
void cleanup_plugins(plugin_handle_t* plugins, int count) {
    if (!plugins) return;
    
    for (int i = 0; i < count; i++) {
        // Close the shared library handle
        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }
        // Free the name string
        if (plugins[i].name) {
            free(plugins[i].name);
        }
    }
    // Free the plugins array
    free(plugins);
}


int main(int argc, char* argv[]) {
// Step 1: Parse Command-Line Arguments

    if (argc < 3) {
        fprintf(stderr, "Error: Missing arguments\n");
        print_usage();
        return 1;
    }

    // Convert queue size argument from string to long using strtol
    char* endptr;
    long queue_size_long = strtol(argv[1], &endptr, 10);

    // Check if the input is not a number
    if (endptr == argv[1]) {
        fprintf(stderr, "Error: Invalid queue size: %s (not a number)\n", argv[1]);
        print_usage();
        return 1;
    }

    // Check if there are extra characters after the number 
    if (*endptr != '\0') {
        fprintf(stderr, "Error: Invalid queue size: %s (contains invalid characters)\n", argv[1]);
        print_usage();
        return 1;
    }

    // Check if the number is within valid range
    if (queue_size_long <= 0) {
        fprintf(stderr, "Error: Queue size must be a positive integer\n");
        print_usage();
        return 1;
    }

    // Cast to int after validation
    int queue_size = (int) queue_size_long;

    // Calculate number of plugins from command line
    int plugin_count = argc - 2;
    
    // Allocate array to hold plugin handles
    plugin_handle_t* plugins = calloc(plugin_count, sizeof(plugin_handle_t));
    if (!plugins) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return 1;
    }

// Step 2: Load Plugin Shared Objects

    for (int i = 0; i < plugin_count; i++) {
        
        // Store plugin name
        plugins[i].name = malloc(strlen(argv[i + 2]) + 1);
        if (!plugins[i].name) {
            fprintf(stderr, "Error: Memory allocation failed for plugin name\n");
            cleanup_plugins(plugins, i);
            return 1;
        }
        strcpy(plugins[i].name, argv[i + 2]);

        // Load the shared library
        char filename[256];
        plugins[i].handle = try_open_plugin(argv[i + 2], filename, sizeof(filename));
        if (!plugins[i].handle) {
            fprintf(stderr, "Error loading plugin %s: %s\n", argv[i + 2], dlerror());
            print_usage();
            cleanup_plugins(plugins, i + 1); // Clean up including current plugin
            return 1;
        }

        // Extract all required function pointers from the plugin
        plugins[i].init = (plugin_init_func_t)dlsym(plugins[i].handle, "plugin_init");
        plugins[i].fini = (plugin_fini_func_t)dlsym(plugins[i].handle, "plugin_fini");
        plugins[i].place_work = (plugin_place_work_func_t)dlsym(plugins[i].handle, "plugin_place_work");
        plugins[i].attach = (plugin_attach_func_t)dlsym(plugins[i].handle, "plugin_attach");
        plugins[i].wait_finished = (plugin_wait_finished_func_t)dlsym(plugins[i].handle, "plugin_wait_finished");
        plugins[i].get_name = (plugin_get_name_func_t)dlsym(plugins[i].handle, "plugin_get_name");

        // Check for any symbol loading errors
        if (!plugins[i].init || !plugins[i].fini || !plugins[i].place_work || 
            !plugins[i].attach || !plugins[i].wait_finished || !plugins[i].get_name) {
            fprintf(stderr, "Error: Plugin %s is missing required interface\n", argv[i + 2]);
            dlclose(plugins[i].handle);
            print_usage();
            cleanup_plugins(plugins, i + 1);
            return 1;
        }
    }

// Step 3: Initialize Plugins

    for (int i = 0; i < plugin_count; i++) {
        const char* err = plugins[i].init(queue_size);
        if (err != NULL) {
            fprintf(stderr, "Error initializing plugin %s: %s\n", plugins[i].name, err);

            // Finalize all successfully initialized plugins
            for (int j = 0; j < i; j++) {
                const char* fini_err = plugins[j].fini();
                if (fini_err) {
                    fprintf(stderr, "Warning: Error finalizing plugin %s: %s\n", 
                            plugins[j].name, fini_err);
                }
            }

            // Release all plugin resources
            cleanup_plugins(plugins, plugin_count);
            return 2;
        }
    }

// Step 4: Attach Plugins Together

    for (int i = 0; i < plugin_count - 1; i++) {
        // Connect each plugin to the next one in the chain
        plugins[i].attach(plugins[i + 1].place_work);
    }

// Step 5: Read Input from STDIN

    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), stdin) != NULL) {

        // Remove trailing newline if present
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Send the line to the first plugin
        const char* error = plugins[0].place_work(line);
        if (error != NULL) {
            fprintf(stderr, "Error placing work: %s\n", error);
            break;
        }

        // Check if this was the END signal
        if (strcmp(line, "<END>") == 0) {
            break;
        }
    }

// Step 6: Wait for Plugins to Finish

    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].wait_finished();
        if (error != NULL) {
            fprintf(stderr, "Warning: Error waiting for plugin %s: %s\n", 
                    plugins[i].name, error);
        }
    }

// Step 7: Cleanup

    // Finalize plugins in order
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].fini();
        if (error != NULL) {
            fprintf(stderr, "Warning: Error finalizing plugin %s: %s\n", 
                    plugins[i].name, error);
        }
    }

    // Clean up resources (handles and memory)
    cleanup_plugins(plugins, plugin_count);

// Step 8: Print Final Message

    printf("Pipeline shutdown complete\n");
    
    return 0;  // Success
}