#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <mutex>
#include <zlib.h>

// Constants
constexpr size_t CHUNK_SIZE = 16384;  // 16 KB buffer size

// Mutex for thread-safe output
std::mutex output_mutex;

// Function to compress a block of data using zlib
std::vector<char> compressBlock(const std::vector<char>& data) {
    if (data.empty()) return {};  // No need to process empty input

    // Allocate space for compressed data (estimate size as twice the input size)
    std::vector<char> compressedData(data.size() * 2);
    uLongf compressedSize = compressedData.size();

    // Perform compression
    if (compress(reinterpret_cast<Bytef*>(compressedData.data()), &compressedSize,
                 reinterpret_cast<const Bytef*>(data.data()), data.size()) != Z_OK) {
        throw std::runtime_error("Compression error: Data could not be compressed.");
    }

    // Resize the vector to the actual compressed size
    compressedData.resize(compressedSize);
    return compressedData;
}

// Function to decompress a block of data using zlib
std::vector<char> decompressBlock(const std::vector<char>& compressedData, uLong originalSize) {
    if (compressedData.empty()) return {};  // Avoid unnecessary processing

    // Allocate space for decompressed data
    std::vector<char> decompressedData(originalSize);
    uLongf decompressedSize = originalSize;

    // Perform decompression
    if (uncompress(reinterpret_cast<Bytef*>(decompressedData.data()), &decompressedSize,
                   reinterpret_cast<const Bytef*>(compressedData.data()), compressedData.size()) != Z_OK) {
        throw std::runtime_error("Decompression error: Data could not be decompressed.");
    }

    // Resize the vector to the actual decompressed size
    decompressedData.resize(decompressedSize);
    return decompressedData;
}

// Function to compress a file using multithreading
void compressFile(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream inFile(inputFile, std::ios::binary);
    std::ofstream outFile(outputFile, std::ios::binary);

    // Check if files are open
    if (!inFile.is_open()) {
        throw std::runtime_error("File error: Unable to open input file: " + inputFile);
    }
    if (!outFile.is_open()) {
        throw std::runtime_error("File error: Unable to open output file: " + outputFile);
    }

    // Write a simple file header for metadata
    outFile.write("CMP1", 4);  // Magic number for compressed file format

    std::vector<std::thread> threads;
    std::vector<std::vector<char>> compressedChunks;
    std::mutex compressionMutex;
    std::exception_ptr threadException = nullptr;

    // Read and compress file in chunks
    while (!inFile.eof()) {
        std::vector<char> buffer(CHUNK_SIZE);
        inFile.read(buffer.data(), CHUNK_SIZE);
        size_t bytesRead = inFile.gcount();
        if (bytesRead == 0) break;

        buffer.resize(bytesRead);  // Adjust size for actual data

        // Launch a thread to compress the chunk
        threads.emplace_back([buffer, &compressedChunks, &compressionMutex, &threadException]() {
            try {
                auto compressed = compressBlock(buffer);
                std::lock_guard<std::mutex> lock(compressionMutex);
                compressedChunks.push_back(std::move(compressed));
            } catch (...) {
                std::lock_guard<std::mutex> lock(output_mutex);
                threadException = std::current_exception();
            }
        });
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Rethrow any exceptions that occurred in threads
    if (threadException) {
        std::rethrow_exception(threadException);
    }

    // Write compressed data to the output file
    for (const auto& chunk : compressedChunks) {
        uLong chunkSize = chunk.size();
        outFile.write(reinterpret_cast<const char*>(&chunkSize), sizeof(chunkSize));
        outFile.write(chunk.data(), chunkSize);
    }
}

// Function to decompress a file using multithreading
void decompressFile(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream inFile(inputFile, std::ios::binary);
    std::ofstream outFile(outputFile, std::ios::binary);

    // Check if files are open
    if (!inFile.is_open()) {
        throw std::runtime_error("File error: Unable to open input file: " + inputFile);
    }
    if (!outFile.is_open()) {
        throw std::runtime_error("File error: Unable to open output file: " + outputFile);
    }

    // Verify file header
    char header[4];
    inFile.read(header, 4);
    if (std::string(header, 4) != "CMP1") {
        throw std::runtime_error("File error: Invalid compressed file format.");
    }

    std::vector<std::thread> threads;
    std::vector<std::vector<char>> decompressedChunks;
    std::mutex decompressionMutex;
    std::exception_ptr threadException = nullptr;

    // Read and decompress file in chunks
    while (!inFile.eof()) {
        uLong chunkSize;
        inFile.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));

        if (inFile.eof()) break;  // Ensure we don't process partial reads

        std::vector<char> compressedBuffer(chunkSize);
        inFile.read(compressedBuffer.data(), chunkSize);

        if (inFile.gcount() != chunkSize) {
            throw std::runtime_error("File error: Corrupted compressed file.");
        }

        // Launch a thread to decompress the chunk
        threads.emplace_back([compressedBuffer, &decompressedChunks, &decompressionMutex, &threadException]() {
            try {
                auto decompressed = decompressBlock(compressedBuffer, CHUNK_SIZE);
                std::lock_guard<std::mutex> lock(decompressionMutex);
                decompressedChunks.push_back(std::move(decompressed));
            } catch (...) {
                std::lock_guard<std::mutex> lock(output_mutex);
                threadException = std::current_exception();
            }
        });
    }

    // Wait for all threads to finish
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // Rethrow any exceptions that occurred in threads
    if (threadException) {
        std::rethrow_exception(threadException);
    }

    // Write decompressed data to the output file
    for (const auto& chunk : decompressedChunks) {
        outFile.write(chunk.data(), chunk.size());
    }
}

// Main function
int main() {
    try {
        std::string inputFile = "input.txt";
        std::string compressedFile = "compressed.dat";
        std::string decompressedFile = "decompressed.txt";

        std::cout << "Compressing file...\n";
        compressFile(inputFile, compressedFile);
        std::cout << "Compression complete!\n";

        std::cout << "Decompressing file...\n";
        decompressFile(compressedFile, decompressedFile);
        std::cout << "Decompression complete!\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
