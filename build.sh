#!/bin/bash

# Exit on any error
set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color


# Functions to print colored output
print_status()  { echo -e "${GREEN}[BUILD]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error()   { echo -e "${RED}[ERROR]${NC} $1"; }

# Create output directory
print_status "Creating output directory..."
mkdir -p output

# Build Main Application
print_status "Building main executable..."
gcc -o output/analyzer main.c -ldl || {
    print_error "Failed to build main executable"
    exit 1
}
print_status "Main executable built successfully"

# Building Plugins
print_status "Starting plugin compilation..."

# Find all .c files in plugins directory excluding subdirectories and plugin_common.c
plugin_count=0
for plugin_file in plugins/*.c; do
    # Check if any .c files exist
    if [ ! -f "$plugin_file" ]; then
        print_warning "No plugin source files found in plugins directory"
        break
    fi
    
    # Skip plugin_common.c
    if [[ "$(basename "$plugin_file")" == "plugin_common.c" ]]; then
        continue
    fi
    
    # Extract plugin name from filename 
    plugin_name=$(basename "$plugin_file" .c)
    
    print_status "Building plugin: $plugin_name"
    
    # Compile the plugin using the exact command structure from assignment
    gcc -fPIC -shared -o output/${plugin_name}.so \
        plugins/${plugin_name}.c \
        plugins/plugin_common.c \
        plugins/sync/monitor.c \
        plugins/sync/consumer_producer.c \
        -ldl -lpthread || {
        print_error "Failed to build $plugin_name"
        exit 1
    }
    
    plugin_count=$((plugin_count + 1))
done

# Final summary
echo ""
print_status "Build complete!"
print_status "Built $plugin_count plugins"
print_status "Output files are in the 'output' directory:"
echo "  - analyzer (main executable)"
for so_file in output/*.so; do
    if [ -f "$so_file" ]; then
        echo "  - $(basename "$so_file")"
    fi
done