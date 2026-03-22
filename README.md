# Multithreaded Modular Pipeline System (C)

A modular, multithreaded string processing pipeline implemented in C, using dynamic plugin loading, producer-consumer synchronization, and custom monitor-based concurrency control.

## Overview

This project implements a flexible pipeline architecture where each stage is a dynamically loaded plugin running in its own thread. Input strings flow through a chain of plugins, each performing a transformation or action.

The system demonstrates core operating systems concepts including:
- Multithreading and synchronization
- Producer-consumer pattern with bounded queues
- Dynamic loading using `dlopen` and `dlsym`
- Modular system design with plugin-based architecture

## Architecture

- The main application dynamically loads plugins at runtime
- Each plugin runs in its own thread
- Communication between plugins is done via thread-safe bounded queues
- A custom monitor abstraction is used to prevent missed signals
- The pipeline processes input from `stdin` until receiving `<END>`

## Supported Plugins

- `logger` – prints strings
- `typewriter` – prints characters with delay
- `uppercaser` – converts to uppercase
- `rotator` – rotates characters
- `flipper` – reverses string
- `expander` – adds spaces between characters

## Build

    ./build.sh

## Run

    ./output/analyzer <queue_size> <plugin1> <plugin2> ...

## Example

    echo "hello" | ./output/analyzer 20 uppercaser rotator logger

## Output

    [logger] OHELL

## Testing

Run the full test suite:

    ./test.sh

The test suite includes:
- Positive and negative test cases
- Stress tests
- Edge cases
- Queue overflow scenarios

## Key Implementation Details

- Custom monitor implementation to avoid lost wakeups
- Circular buffer-based producer-consumer queue
- Graceful shutdown using <END> signal propagation
- Thread-safe plugin chaining via function pointers

## Technologies

- C
- POSIX Threads (pthread)
- Dynamic linking (dlopen, dlsym)
- Bash
