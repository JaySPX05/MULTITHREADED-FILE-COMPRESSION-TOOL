# MULTITHREADED-FILE-COMPRESSION-TOOL

*COMPANY*: CODTECH IT SOLUTIONS

*NAME*: JAY PANDIAN

*INTERN ID*: CT04WX43

*DOMAIN*: C++ PROGRAMMING

*DURATION*: 4 WEEKS

*MENTOR*: NEELA SANTOSH

This C++ program is designed to compress and decompress files efficiently using the zlib library. What makes it special is that it uses multithreading, meaning it can handle multiple parts of a file at the same time, making the process much faster than traditional methods.

How It Works:
Reading the File in Chunks
Instead of processing the entire file at once, the program splits it into smaller chunks and processes them individually.

Compressing with Multiple Threads:
Each chunk is assigned to a separate thread, allowing multiple parts of the file to be compressed simultaneously.
Uses zlib::deflate() to compress data.
The compressed data is stored in memory before being written to the output file.

Handling File Writing Safely:
Since multiple threads are working at once, there’s a chance they could interfere with each other when writing to the output file.
To prevent this, a mutex (std::mutex) is used to ensure only one thread writes at a time.

Decompression Process:
The program reads the compressed file, again in chunks.
Each chunk is decompressed using zlib::inflate() in parallel.
The original file is reconstructed and saved.

Key Components of the Code:
compress_chunk()  Handles compression for a single chunk.
decompress_chunk()  Decompresses a single chunk.
compress_file()  Splits the file, compresses chunks using threads, and writes the compressed data.
decompress_file() Reads compressed data, decompresses in parallel, and writes the output.
main()  Calls compression and decompression functions for testing.

Why It’s Useful:
Faster processing thanks to multithreading.
Efficient file handling with chunk-based compression.
Thread-safe writing using std::mutex.
If you’ve ever waited too long for a large file to compress, this approach can significantly speed things up by taking advantage of your computer’s multiple cores.

