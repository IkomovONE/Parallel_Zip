# Project 5: Parallel Zip

**Authors**: Daniil Komov, Igor Zimarev

## Overview
For this mini-project, we created two C programs for compressing and decompressing files using RLE encoding and parallel multi-threading:

- **pzip.c**: Compresses one or multiple files (best with .txt files) using parallel threads.
- **punzip.c**: Decompresses one or multiple compressed files using parallel threads.

## Compilation
Compile the programs using `gcc`:
```bash
gcc -o pzip pzip.c -Wall -Werror
gcc -o punzip punzip.c -Wall -Werror
```
Or use the pre-compiled executables in directory.

## Usage
### Compression (`pzip`)
Compress a single file:
```bash
./pzip inputFile > outputFile.z
```

Compress multiple files:
```bash
./pzip inputFile1 inputFile2 ... > outputFile.z
```
> Note: The output file should have a `.z` extension.

### Decompression (`punzip`)
Decompress a single file:
```bash
./punzip compressed_inputFile > decompressed_outputFile.txt
```

Decompress multiple files:
```bash
./punzip compressed_inputFile1 compressed_inputFile2 ... > decompressed_outputFile.txt
```

## Error Handling
- Ensure files have **Read & Write permissions**.
- If a file is unavailable or corrupt, you will see: `error: cannot open file '<file>'`.
- If there is a memory allocation issue, the error `malloc failed` will appear.

## Limitations
- No inherent file size limitations, but very large files may impact system performance or fail due to memory constraints.

## Project Considerations
### Parallelization
- Files are divided into segments, each processed by a separate thread.
- The number of threads matches the number of available CPU cores.

### Determining how many threads to create
- The program reads the number of system processors available, then accordingly creates as many threads as the number of these processors.

### Efficiency
- Memory mapping (`mmap()`) is used for efficient file access.
- Each segment is encoded using simple looping commands, enhancing CPU efficiency.

### Accessing the input file efficiently
- To access input files the program simply opens the file, then uses mmap() command to map the file into the memory address space.

## Execution examples
### Compression
```bash
./pzip example_input.txt > compressed_output.z
```

### Decompression
```bash
./punzip compressed_output.z > decompressed_output.txt
```
