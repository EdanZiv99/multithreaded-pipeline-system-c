#!/bin/bash

# test.sh - Comprehensive test suite for Plugin Pipeline System
# Required for assignment submission
# Tests both positive and negative cases as specified in requirements

# Don't use set -e to prevent premature script termination

# Colors for clear output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Helper functions
print_status() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[PASS]${NC} $1"; }
print_failure() { echo -e "${RED}[FAIL]${NC} $1"; }
print_section() { echo -e "\n${YELLOW}=== $1 ===${NC}"; }

# Main test function
run_test() {
    local test_name="$1"
    local expected_output="$2"
    local input="$3"
    shift 3
    local cmd="$@"
    
    ((TESTS_RUN++))
    print_status "Running: $test_name"
    
    # Run the command and capture output
    if [ -n "$input" ]; then
        actual_output=$(echo -e "$input" | eval "$cmd" 2>&1)
        exit_code=$?
    else
        actual_output=$(eval "$cmd" 2>&1)
        exit_code=$?
    fi
    
    # Check if output contains expected text
    if echo "$actual_output" | grep -q "$expected_output"; then
        print_success "$test_name"
        ((TESTS_PASSED++))
        return 0
    else
        print_failure "$test_name"
        ((TESTS_FAILED++))
        echo "  Expected to find: '$expected_output'"
        echo "  Actual output: '$actual_output'"
        echo "  Exit code: $exit_code"
        return 1
    fi
}

# Test function for exact exit codes (for error cases)
run_exit_test() {
    local test_name="$1"
    local expected_exit="$2"
    local input="$3"
    shift 3
    local cmd="$@"
    
    ((TESTS_RUN++))
    print_status "Running: $test_name"
    
    # Run command and check exit code
    if [ -n "$input" ]; then
        output=$(echo -e "$input" | eval "$cmd" 2>&1)
        actual_exit=$?
    else
        output=$(eval "$cmd" 2>&1)
        actual_exit=$?
    fi
    
    if [ $actual_exit -eq $expected_exit ]; then
        print_success "$test_name (exit code: $actual_exit)"
        ((TESTS_PASSED++))
        return 0
    else
        print_failure "$test_name"
        ((TESTS_FAILED++))
        echo "  Expected exit code: $expected_exit"
        echo "  Actual exit code: $actual_exit"
        echo "  Output: $output"
        return 1
    fi
}

echo "======================================================="
echo "PLUGIN PIPELINE SYSTEM - COMPREHENSIVE TEST SUITE"
echo "======================================================="

# Step 1: Build the project
print_section "BUILDING PROJECT"
print_status "Calling build.sh to compile main program and all plugins"

echo "DEBUG: About to run build.sh"
if ./build.sh > /dev/null 2>&1; then
    print_success "Build completed successfully"
    echo "DEBUG: Build succeeded"
else
    print_failure "Build failed - cannot proceed with tests"
    echo "DEBUG: Build failed, running build.sh again to show errors:"
    ./build.sh
    exit 1
fi

echo "DEBUG: Checking for output directory"
if [ ! -d "./output" ]; then
    print_failure "Output directory not found"
    echo "DEBUG: No output directory found"
    exit 1
fi

echo "DEBUG: Checking for main executable"

# Verify build artifacts exist
if [ ! -f "./output/analyzer" ] || [ ! -x "./output/analyzer" ]; then
    print_failure "Main executable not found or not executable"
    exit 1
fi

# Check that all required plugins were built
for plugin in logger typewriter uppercaser rotator flipper expander; do
    if [ ! -f "./output/$plugin.so" ]; then
        print_failure "Plugin $plugin.so not found"
        exit 1
    fi
done

print_success "All build artifacts verified"

# Change to output directory for testing
cd output

print_section "NEGATIVE TEST CASES (Error Handling)"

# Test missing arguments
run_exit_test "No arguments provided" 1 "" "./analyzer"

# Test insufficient arguments
run_exit_test "Only queue size provided" 1 "" "./analyzer 10"

# Test invalid queue sizes
run_exit_test "Negative queue size" 1 "" "./analyzer -5 logger"
run_exit_test "Zero queue size" 1 "" "./analyzer 0 logger"
run_exit_test "Non-numeric queue size" 1 "" "./analyzer abc logger"
run_exit_test "Queue size with invalid characters" 1 "" "./analyzer 10abc logger"

# Test non-existent plugin
run_exit_test "Non-existent plugin" 1 "" "./analyzer 10 nonexistent"

# Test plugin loading errors
run_exit_test "Multiple non-existent plugins" 1 "" "./analyzer 10 fake1 fake2"

print_section "POSITIVE TEST CASES (Correct Functionality)"

# Basic single plugin tests
run_test "Basic logger functionality" "Pipeline shutdown complete" "hello\n<END>" "./analyzer 10 logger"
run_test "Basic uppercaser functionality" "Pipeline shutdown complete" "hello\n<END>" "./analyzer 10 uppercaser"
run_test "Basic rotator functionality" "Pipeline shutdown complete" "hello\n<END>" "./analyzer 10 rotator"
run_test "Basic flipper functionality" "Pipeline shutdown complete" "hello\n<END>" "./analyzer 10 flipper"
run_test "Basic expander functionality" "Pipeline shutdown complete" "hello\n<END>" "./analyzer 10 expander"
run_test "Basic typewriter functionality" "Pipeline shutdown complete" "hello\n<END>" "./analyzer 10 typewriter"

# Test plugin transformations with logger to verify output
run_test "Uppercaser transformation" "\[logger\] HELLO" "hello\n<END>" "./analyzer 10 uppercaser logger"
run_test "Rotator transformation" "\[logger\] ohell" "hello\n<END>" "./analyzer 10 rotator logger"
run_test "Flipper transformation" "\[logger\] olleh" "hello\n<END>" "./analyzer 10 flipper logger"
run_test "Expander transformation" "\[logger\] h e l l o" "hello\n<END>" "./analyzer 10 expander logger"

print_section "PLUGIN CHAIN TESTS"

# Test two-plugin chains
run_test "Two plugins: uppercaser -> logger" "\[logger\] HELLO" "hello\n<END>" "./analyzer 10 uppercaser logger"
run_test "Two plugins: rotator -> logger" "\[logger\] ohell" "hello\n<END>" "./analyzer 10 rotator logger"

# Test three-plugin chains
run_test "Three plugins: uppercaser -> rotator -> logger" "\[logger\] OHELL" "hello\n<END>" "./analyzer 10 uppercaser rotator logger"
run_test "Three plugins: rotator -> flipper -> logger" "\[logger\] lleho" "hello\n<END>" "./analyzer 10 rotator flipper logger"

# Test complex chains
run_test "Complex chain: expander -> uppercaser -> flipper -> logger" "\[logger\] O L L E H" "hello\n<END>" "./analyzer 10 expander uppercaser flipper logger"

print_section "COMPREHENSIVE EDGE CASE TESTS"

# Maximum line length testing
MAX_LINE=$(printf 'x%.0s' {1..1024})
run_test "Maximum line length (1024 chars)" "Pipeline shutdown complete" "$MAX_LINE\n<END>" "./analyzer 10 logger"

# Over maximum line length
OVER_MAX=$(printf 'x%.0s' {1..1025})  
run_test "Over maximum line length (1025 chars)" "Pipeline shutdown complete" "$OVER_MAX\n<END>" "./analyzer 10 logger"

# Memory stress with rapid inputs
RAPID_SEQUENCE=""
for i in {1..50}; do RAPID_SEQUENCE="${RAPID_SEQUENCE}input$i\n"; done
RAPID_SEQUENCE="${RAPID_SEQUENCE}<END>"
run_test "Memory stress - 50 rapid inputs" "Pipeline shutdown complete" "$RAPID_SEQUENCE" "./analyzer 2 logger"

# Threading stress with minimal queue
run_test "Threading stress - minimal queue with complex chain" "Pipeline shutdown complete" "test\n<END>" "./analyzer 1 uppercaser rotator flipper expander logger"

# Multiple END signals
run_test "Multiple END signals handling" "Pipeline shutdown complete" "test\n<END>\n<END>" "./analyzer 10 logger"

# Queue overflow scenarios  
OVERFLOW_INPUT=""
for i in {1..20}; do OVERFLOW_INPUT="${OVERFLOW_INPUT}overflow$i\n"; done
OVERFLOW_INPUT="${OVERFLOW_INPUT}<END>"
run_test "Queue overflow handling" "Pipeline shutdown complete" "$OVERFLOW_INPUT" "./analyzer 1 logger"

# Boundary condition: Single queue slot with chain
run_test "Single queue slot stress test" "Pipeline shutdown complete" "single\nmultiple\nlines\n<END>" "./analyzer 1 uppercaser rotator logger"

# Empty strings
run_test "Empty string input" "Pipeline shutdown complete" "\n<END>" "./analyzer 10 logger"
run_test "Empty string with transformation" "\[logger\]" "\n<END>" "./analyzer 10 uppercaser logger"

# Single characters
run_test "Single character" "\[logger\] A" "a\n<END>" "./analyzer 10 uppercaser logger"
run_test "Single character rotation" "\[logger\] a" "a\n<END>" "./analyzer 10 rotator logger"

# Special characters
run_test "Special characters" "\[logger\] !@#\$%\^&\*\(\)" "!@#\$%^&*()\n<END>" "./analyzer 10 uppercaser logger"
run_test "Numbers" "\[logger\] 12345" "12345\n<END>" "./analyzer 10 uppercaser logger"

# Long strings
LONG_STRING=$(printf 'x%.0s' {1..100})
run_test "Long string (100 chars)" "Pipeline shutdown complete" "$LONG_STRING\n<END>" "./analyzer 10 uppercaser"

# Multiple input lines
run_test "Multiple input lines" "Pipeline shutdown complete" "line1\nline2\nline3\n<END>" "./analyzer 10 logger"

# Immediate END signal
run_test "Immediate END signal" "Pipeline shutdown complete" "<END>" "./analyzer 10 logger"

print_section "QUEUE SIZE VARIATION TESTS"

# Different queue sizes
run_test "Minimum queue size (1)" "\[logger\] HELLO" "hello\n<END>" "./analyzer 1 uppercaser logger"
run_test "Small queue size (2)" "\[logger\] HELLO" "hello\n<END>" "./analyzer 2 uppercaser logger"
run_test "Large queue size (100)" "\[logger\] HELLO" "hello\n<END>" "./analyzer 100 uppercaser logger"

print_section "COMPREHENSIVE TRANSFORMATION TESTS"

# Verify specific transformations
run_test "Rotator: 'abc' becomes 'cab'" "\[logger\] cab" "abc\n<END>" "./analyzer 10 rotator logger"
run_test "Flipper: 'abc' becomes 'cba'" "\[logger\] cba" "abc\n<END>" "./analyzer 10 flipper logger"
run_test "Expander: 'abc' becomes 'a b c'" "\[logger\] a b c" "abc\n<END>" "./analyzer 10 expander logger"

# Combination transformations
run_test "Uppercaser -> Rotator: 'hello' -> 'OHELL'" "\[logger\] OHELL" "hello\n<END>" "./analyzer 10 uppercaser rotator logger"
run_test "Rotator -> Uppercaser: 'hello' -> 'OHELL'" "\[logger\] OHELL" "hello\n<END>" "./analyzer 10 rotator uppercaser logger"
run_test "Flipper -> Rotator: 'hello' -> 'holle'" "\[logger\] holle" "hello\n<END>" "./analyzer 10 flipper rotator logger"

print_section "STRESS TESTS"

# Rapid input processing
RAPID_INPUT=""
for i in {1..10}; do
    RAPID_INPUT="${RAPID_INPUT}input$i\n"
done
RAPID_INPUT="${RAPID_INPUT}<END>"
run_test "Multiple rapid inputs" "Pipeline shutdown complete" "$RAPID_INPUT" "./analyzer 5 logger"

print_section "ASSIGNMENT EXAMPLE VERIFICATION"

# Based on the assignment example showing transformation pipeline
run_test "Assignment example pipeline" "\[logger\] OHELL" "hello\n<END>" "./analyzer 20 uppercaser rotator logger"

print_section "ADDITIONAL VALIDATION TESTS"

# Test that typewriter works with timing (basic check)
run_test "Typewriter with timing" "Pipeline shutdown complete" "hi\n<END>" "./analyzer 10 typewriter"

# Test whitespace handling
run_test "Leading/trailing spaces" "\[logger\]   HELLO  " "  hello  \n<END>" "./analyzer 10 uppercaser logger"
run_test "Tabs and spaces" "Pipeline shutdown complete" "\t  hello  \t\n<END>" "./analyzer 10 logger"

# Test plugin interface compliance by checking shutdown message appears
run_test "Graceful shutdown verification" "Pipeline shutdown complete" "test\n<END>" "./analyzer 10 uppercaser rotator flipper"

# Test mixed content
run_test "Mixed alphanumeric" "\[logger\] HELLO123WORLD" "hello123world\n<END>" "./analyzer 10 uppercaser logger"

print_section "TEST RESULTS SUMMARY"

echo "======================================================="
echo "TOTAL TESTS RUN: $TESTS_RUN"
echo "TESTS PASSED: $TESTS_PASSED"
echo "TESTS FAILED: $TESTS_FAILED"
echo "======================================================="

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}SUCCESS: All tests passed!${NC}"
    echo "Your pipeline system is working correctly."
    exit 0
else
    echo -e "${RED}FAILURE: $TESTS_FAILED test(s) failed.${NC}"
    echo "Please review the failed tests and fix the issues."
    exit 1
fi