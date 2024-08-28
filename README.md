# Cache Simulator in C++

## Overview

This project implements a cache simulator written in C++. The simulator models the behavior of a CPU cache system, providing insights into cache performance by simulating cache hits, misses, and evictions based on various cache parameters and access patterns. The project is designed to help users understand how different cache configurations affect performance in terms of access time and efficiency.

## Features

- **Configurable Cache Parameters:** Supports various cache configurations including:
  - Cache size
  - Block size
  - Associativity (Direct-mapped, Fully associative, Set associative)
  - Replacement policies (LRU, FIFO, Random)
- **Cache Operations:** Simulates typical cache operations including read, write, and eviction.
- **Performance Metrics:** Tracks and displays key metrics such as hit rate, miss rate, and eviction count.
- **Trace File Input:** Allows users to input memory access traces to simulate real-world scenarios.
- **Detailed Output:** Provides a detailed summary of cache performance after each simulation run.

## Usage

1. **Compilation:**
   - Compile the project using `g++` or any compatible C++ compiler:
     ```bash
     g++ -o cache_simulator cache_simulator.cpp
     ```
   - This will create an executable named `cache_simulator`.

2. **Running the Simulator:**
   - Run the simulator with the desired cache configuration and input trace file:
     ```bash
     ./cache_simulator <trace_file> <cache_size> <block_size> <associativity> <replacement_policy>
     ```
   - Example:
     ```bash
     ./cache_simulator trace.txt 1024 64 2 LRU
     ```

   - Parameters:
     - `<trace_file>`: Path to the memory access trace file.
     - `<cache_size>`: Total size of the cache in bytes.
     - `<block_size>`: Size of each cache block in bytes.
     - `<associativity>`: Cache associativity (1 for direct-mapped, N for N-way set associative, 0 for fully associative).
     - `<replacement_policy>`: Cache replacement policy (`LRU`, `FIFO`, `Random`).

3. **Viewing Results:**
   - After running the simulation, the output will display cache performance metrics such as hit rate, miss rate, and the number of evictions.

## Example Trace File Format

The trace file should contain memory addresses to be accessed, one per line, in hexadecimal format. For example:

```
0x1A2B3C
0x1A2B40
0x3F4A1C
0x1A2B3C
```

## Example Command

```sh
./cache_simulator trace.txt 4096 64 4 LRU
```

This command simulates a 4KB cache with 64-byte blocks, 4-way set associativity, and the LRU replacement policy using the memory access trace in `trace.txt`.

## Future Enhancements

- **Multi-level Cache Simulation:** Adding support for simulating multi-level cache hierarchies (L1, L2, L3).
- **Write Policies:** Implementing write-through and write-back policies.
- **Graphical Output:** Providing visual representations of cache performance over time.
- **Cache Coherency:** Exploring cache coherency protocols in a multi-core environment.

