#include "packr.hpp"
#include <sys/stat.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

#define SINFL_IMPLEMENTATION
#define SDEFL_IMPLEMENTATION
#include "sinfl.h"
#include "sdefl.h"

#define COMP_QUALITY 8


/* Helper datatypes */
/* ================ */

// For path types
enum PathType {
    FILE_PATH = 0,
    DIR_PATH,
    UNKNOWN_PATH,
};

/* Helper functions */
/* ================ */

// Check if a given path leads to a file or a directory
PathType get_path_type(std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0)
        return UNKNOWN_PATH;

    if (S_ISREG(buffer.st_mode)) return FILE_PATH;
    if (S_ISDIR(buffer.st_mode)) return DIR_PATH;
    return UNKNOWN_PATH;
}


// Load all filepaths from a given directory
void load_files_from_dir(std::string& path, std::vector<std::string>& files) {
    if (get_path_type(path) == DIR_PATH) {
        // Now loop through the entries in the folder
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            std::string entry_path = entry.path();
            // If this is a file path, store it
            if (get_path_type(entry_path) == FILE_PATH) {
                files.push_back(entry_path);
            }
            // Else explore this directory
            else if (get_path_type(entry_path) == DIR_PATH) {
                load_files_from_dir(entry_path, files);
            }
        }
    }
    else {
        std::runtime_error("error: Given invalid path! Path must be an existing directory");
    }
}

// Compression function
std::vector<char> compress_data(const char* data, uint32_t size, uint32_t& comp_size) {
    thread_local sdefl ctx{};
    int bound = sdefl_bound(size);

    std::vector<char> out(bound);
    comp_size = sdeflate(&ctx, out.data(), data, size, COMP_QUALITY);

    out.resize(comp_size);
    return out;
}

// Decompression function
std::vector<char> decompress_data(const char* comp_data, uint32_t comp_size, uint32_t expected_size)
{
    std::vector<char> out(expected_size);

    int result = sinflate(out.data(), expected_size, comp_data, comp_size);
    if (result < 0) {
        throw std::runtime_error("Decompression failed");
    }

    out.resize(result);
    return out;
}


/* class PackrFile */
/* =============== */
PackrFile::PackrFile(const std::string& path, bool new_file) {
    // Store file path
    this->file_path = path;

    // Check if the file exists
    // We DONT'T want to open an existing file in certain circumstances, or accidentally flush an old file
    bool exists = std::filesystem::exists(path);

    // ERROR if file already exists and new_file = TRUE
    if (new_file && exists) {
        throw std::runtime_error(
            "PackrFile error: file already exists: " + path);
    }

    if (!new_file && !exists) {
        throw std::runtime_error(
            "PackrFile error: file does not exist: " + path);
    }

    if (new_file) {
        // Create new file (fails if exists, per check above)
        std::ofstream create(path, std::ios::binary);
        if (!create) {
            throw std::runtime_error(
                "PackrFile error: failed to create file: " + path);
        }
        create.close();

        // Open for read/write
        file.open(path, std::ios::in | std::ios::out | std::ios::binary);

        // Initialize the header
        std::memset(&header, 0, sizeof(FileHeader));
        strcpy(header.version, PACKR_VERSION);
        header.chunk_count = 0;

        // Write the header
        file.seekp(0, std::ios::beg);
        file.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));
        file.flush();

        std::cout << "Created .packr file!" << std::endl;
    }
    else {
        // Read existing file
        file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error(
                "PackrFile error: failed to open existing file: " + path);
        }

        // Read header
        file.seekg(0, std::ios::beg);
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));

        // Now read every chunk
        chunks.resize(header.chunk_count);
        std::cout << "Found " << header.chunk_count << " chunks. Loading them..." << std::endl;

        for(int x = 0; x < header.chunk_count; x++) {
            // First read the header
            file.read(reinterpret_cast<char*>(&chunks[x].header), sizeof(chunks[x].header));
            // Then the data
            chunks[x].data.resize(chunks[x].header.comp_size);
            file.read(chunks[x].data.data(), chunks[x].header.comp_size);
        }

        std::cout << "Loaded all chunks!" << std::endl;
    }

    // Close file - we'll open it later for writing all our data
    file.close();
}


PackrFile::~PackrFile() {
    file.close();
}


void PackrFile::add_chunk(DataChunk& chunk) {
    chunk.data = compress_data(
        chunk.data.data(),
        chunk.header.base_size,
        chunk.header.comp_size
    );

    add_compressed_chunk(chunk);
}
void PackrFile::add_compressed_chunk(DataChunk& chunk) {
    chunks.push_back(chunk);
}

void PackrFile::flush() {
    // Open file for writing
    file.open(file_path, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("PackrFile error: failed to open file for writing: " + file_path);
    }

    // Update header with current chunk count
    header.chunk_count = static_cast<uint32_t>(chunks.size());

    // Write the header
    file.write(reinterpret_cast<const char*>(&header), sizeof(FileHeader));

    // Write each chunk
    for (const auto& chunk : chunks) {
        // Write chunk header
        file.write(reinterpret_cast<const char*>(&chunk.header), sizeof(DataHeader));
        // Write chunk data
        file.write(chunk.data.data(), chunk.header.comp_size);
    }

    file.flush();
    file.close();

    std::cout << "Wrote " << chunks.size() << " chunks to " << file_path << std::endl;
}

/* class Packr */
/* =========== */

void Packr::archive(std::string& in_path, std::string& out_path) {
    // First load directories recursively
    std::vector<std::string> files;
    load_files_from_dir(in_path, files);

    // DEBUG: Print out files
    for (int i = 0; i < files.size(); i++) {
        std::cout << files[i] << std::endl;
    }

    // Create a new .packr file
    PackrFile file(out_path, true);

    // Now create each chunk
    for (const std::string& file_path : files) {
        // Open file in binary mode
        std::ifstream in(file_path, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            std::cerr << "Failed to open: " << file_path << std::endl;
            continue;
        }

        // Get file size
        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);

        // Create header
        DataHeader header{};
        std::memset(header.alias, 0, sizeof(header.alias));

        // Store relative path or filename as alias
        std::strncpy(header.alias, file_path.c_str(), sizeof(header.alias) - 1);

        header.base_size = static_cast<uint32_t>(size);
        header.comp_size = static_cast<uint32_t>(size); // same as base for archive (no compression)

        // Create chunk and read data directly into it
        DataChunk chunk;
        chunk.header = header;
        chunk.data.resize(size);
        
        if (!in.read(chunk.data.data(), size)) {
            std::cerr << "Failed to read: " << file_path << std::endl;
            continue;
        }

        // Push chunk into PackrFile (uncompressed)
        file.add_compressed_chunk(chunk);
    }

    // Write all chunks to disk
    file.flush();
}

void Packr::compress(std::string& in_path, std::string& out_path) {
    // First load directories recursively
    std::vector<std::string> files;
    load_files_from_dir(in_path, files);

    // Create a new .packr file
    PackrFile file(out_path, true);

    // Now create each chunk
    for (const std::string& file_path : files) {
        // Open file in binary mode
        std::ifstream in(file_path, std::ios::binary | std::ios::ate);
        if (!in.is_open()) {
            std::cerr << "Failed to open: " << file_path << std::endl;
            continue;
        }

        // Get file size
        std::streamsize size = in.tellg();
        in.seekg(0, std::ios::beg);

        // Create header
        DataHeader header{};
        std::memset(header.alias, 0, sizeof(header.alias));

        // Store relative path or filename as alias
        std::strncpy(header.alias, file_path.c_str(), sizeof(header.alias) - 1);

        header.base_size = static_cast<uint32_t>(size);
        header.comp_size = static_cast<uint32_t>(size); // will be updated by add_chunk()

        // Create chunk and read data directly into it
        DataChunk chunk;
        chunk.header = header;
        chunk.data.resize(size);
        
        if (!in.read(chunk.data.data(), size)) {
            std::cerr << "Failed to read: " << file_path << std::endl;
            continue;
        }

        // Push chunk into PackrFile (will be compressed)
        file.add_chunk(chunk);
    }

    // Write all chunks to disk
    file.flush();
}

void Packr::compress_parallel(std::string& in_path,
                              std::string& out_path,
                              int num_threads) {
    // First load directories recursively
    std::vector<std::string> files;
    load_files_from_dir(in_path, files);

    // Create a new .packr file
    PackrFile file(out_path, true);

    // Push all files into queue
    std::queue<std::string> work_queue;
    for (auto& f : files) work_queue.push(f);

    // Mutexes for locking critical section
    std::mutex queue_mutex;
    std::mutex packr_mutex;
    std::atomic<bool> done{false};

    // Worker function
    auto worker = [&]() {
        while (true) {
            std::string file_path;
            // (CRITICAL) Remove file name from queue
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (work_queue.empty())
                    return;
                file_path = work_queue.front();
                work_queue.pop();
            }

            // Load file data
            std::ifstream in(file_path, std::ios::binary | std::ios::ate);
            if (!in.is_open()) continue;

            std::streamsize size = in.tellg();
            in.seekg(0, std::ios::beg);

            // Directly compress chunk inside memory
            DataChunk chunk;
            std::memset(chunk.header.alias, 0, sizeof(chunk.header.alias));
            std::strncpy(chunk.header.alias, file_path.c_str(),
                        sizeof(chunk.header.alias) - 1);

            chunk.header.base_size = static_cast<uint32_t>(size);
            chunk.header.comp_size = static_cast<uint32_t>(size);
            chunk.data.resize(size);

            if (!in.read(chunk.data.data(), size)) continue;

            chunk.data = compress_data(
                chunk.data.data(),
                chunk.header.base_size,
                chunk.header.comp_size
            );

            // (CRITICAL) Add compressed chunk back to files
            {
                std::lock_guard<std::mutex> lock(packr_mutex);
                file.add_compressed_chunk(chunk);
            }
        }
    };


    // Spawn all threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker);
    }

    // Reap all threads
    for (auto& t : threads) {
        t.join();
    }

    // Write to disk
    file.flush();
}

void Packr::decompress(std::string& in_path, std::string& out_path) {
    // Open existing .packr file
    PackrFile packr_file(in_path, false);

    // Create output directory if it doesn't exist
    std::filesystem::create_directories(out_path);

    // Get all chunks from the packr file
    const auto& chunks = packr_file.get_chunks();

    std::cout << "Decompressing " << chunks.size() << " files..." << std::endl;

    for (const auto& chunk : chunks) {
        // Get the original filename from alias
        std::string alias(chunk.header.alias);

        // Extract just the filename (remove any path prefix)
        std::filesystem::path relative_path(alias);

        // If alias is absolute, strip root to avoid writing outside out_path
        if (relative_path.is_absolute()) {
            relative_path = relative_path.lexically_relative(
                relative_path.root_path());
        }

        std::filesystem::path output_file =
            std::filesystem::path(out_path) / relative_path;


        // Create parent directories if needed
        std::filesystem::create_directories(output_file.parent_path());

        // Decompress the data
        std::vector<char> decompressed;
        if (chunk.header.base_size != chunk.header.comp_size) {
            // Data is compressed, decompress it
            decompressed = decompress_data(
                chunk.data.data(),
                chunk.header.comp_size,
                chunk.header.base_size
            );
        } else {
            // Data is not compressed (from archive())
            decompressed = chunk.data;
        }

        // Write to file
        std::ofstream out(output_file, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Failed to create: " << output_file << std::endl;
            continue;
        }

        out.write(decompressed.data(), decompressed.size());
        out.close();

        std::cout << "Extracted: " << output_file << " (" << decompressed.size() << " bytes)" << std::endl;
    }

    std::cout << "Decompression complete!" << std::endl;
}

void Packr::unarchive(std::string& in_path, std::string& out_path) {
    // Open existing .packr file
    PackrFile packr_file(in_path, false);

    // Create output directory if it doesn't exist
    std::filesystem::create_directories(out_path);

    // Get all chunks from the packr file
    const auto& chunks = packr_file.get_chunks();

    std::cout << "Unarchiving " << chunks.size() << " files..." << std::endl;

    for (const auto& chunk : chunks) {
        // Get the original filename from alias
        std::string alias(chunk.header.alias);

        // Extract just the filename (remove any path prefix)
        std::filesystem::path relative_path(alias);

        // If alias is absolute, strip root to avoid writing outside out_path
        if (relative_path.is_absolute()) {
            relative_path = relative_path.lexically_relative(
                relative_path.root_path());
        }

        std::filesystem::path output_file =
            std::filesystem::path(out_path) / relative_path;


        // Create parent directories if needed
        std::filesystem::create_directories(output_file.parent_path());

        // Write data directly (no decompression needed for archived files)
        std::ofstream out(output_file, std::ios::binary);
        if (!out.is_open()) {
            std::cerr << "Failed to create: " << output_file << std::endl;
            continue;
        }

        out.write(chunk.data.data(), chunk.header.base_size);
        out.close();

        std::cout << "Extracted: " << output_file << " (" << chunk.header.base_size << " bytes)" << std::endl;
    }

    std::cout << "Unarchive complete!" << std::endl;
}


void Packr::decompress_parallel(std::string& in_path,
                                std::string& out_path,
                                int num_threads) {
    // Open existing .packr file
    PackrFile packr_file(in_path, false);

    // Create output directory if it doesn't exist
    std::filesystem::create_directories(out_path);

    // Get all chunks from the packr file
    const auto& chunks = packr_file.get_chunks();

    std::cout << "Decompressing " << chunks.size() << " files with " 
              << num_threads << " threads..." << std::endl;

    // Create a queue of chunk indices to process
    std::queue<size_t> work_queue;
    for (size_t i = 0; i < chunks.size(); i++) {
        work_queue.push(i);
    }

    // Mutex for queue access
    std::mutex queue_mutex;

    // Worker function
    auto worker = [&]() {
        while (true) {
            size_t chunk_idx;

            // (CRITICAL) Get next chunk index from queue
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                if (work_queue.empty())
                    return;
                chunk_idx = work_queue.front();
                work_queue.pop();
            }

            const auto& chunk = chunks[chunk_idx];

            // Get the original filename from alias
            std::string alias(chunk.header.alias);
            
            // Extract just the filename (remove any path prefix)
            std::filesystem::path original_path(alias);
            std::string filename = original_path.filename().string();

            // Build output path
            std::filesystem::path output_file = std::filesystem::path(out_path) / filename;

            // Decompress the data
            std::vector<char> decompressed;
            if (chunk.header.base_size != chunk.header.comp_size) {
                // Data is compressed, decompress it
                decompressed = decompress_data(
                    chunk.data.data(),
                    chunk.header.comp_size,
                    chunk.header.base_size
                );
            } else {
                // Data is not compressed (from archive())
                decompressed = chunk.data;
            }

            // Write to file (file I/O is thread-safe for different files)
            std::ofstream out(output_file, std::ios::binary);
            if (!out.is_open()) {
                std::cerr << "Failed to create: " << output_file << std::endl;
                continue;
            }

            out.write(decompressed.data(), decompressed.size());
            out.close();
        }
    };

    // Spawn all threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker);
    }

    // Join all threads
    for (auto& t : threads) {
        t.join();
    }

    std::cout << "Parallel decompression complete!" << std::endl;
}
