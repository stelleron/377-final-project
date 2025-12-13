#include "packr.hpp"
#include <sys/stat.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

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
    sdefl ctx{};
    int bound = sdefl_bound(size);

    std::vector<char> out(bound);
    comp_size = sdeflate(&ctx, out.data(), data, size, COMP_QUALITY);

    out.resize(comp_size); // shrink to actual size
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

        // Allocate buffer
        char* buffer = new char[size];

        // Read file data
        if (!in.read(buffer, size)) {
            std::cerr << "Failed to read: " << file_path << std::endl;
            delete[] buffer;
            continue;
        }

        // Create header
        DataHeader header{};
        std::memset(header.alias, 0, sizeof(header.alias));

        // Store relative path or filename as alias
        std::strncpy(header.alias, file_path.c_str(), sizeof(header.alias) - 1);

        header.base_size = static_cast<int>(size);
        header.comp_size = static_cast<int>(size); // update later if compressed

        // Create chunk
        DataChunk chunk;
        chunk.header = header;
        chunk.data.resize(size);
        in.read(chunk.data.data(), size);

        // Push chunk into PackrFile (uncompressed)
        file.add_compressed_chunk(chunk);
    }
}

void Packr::compress(std::string& in_path, std::string& out_path) {
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

        // Allocate buffer
        char* buffer = new char[size];

        // Read file data
        if (!in.read(buffer, size)) {
            std::cerr << "Failed to read: " << file_path << std::endl;
            delete[] buffer;
            continue;
        }

        // Create header
        DataHeader header{};
        std::memset(header.alias, 0, sizeof(header.alias));

        // Store relative path or filename as alias
        std::strncpy(header.alias, file_path.c_str(), sizeof(header.alias) - 1);

        header.base_size = static_cast<int>(size);
        header.comp_size = static_cast<int>(size); // update later if compressed

        // Create chunk
        DataChunk chunk;
        chunk.header = header;
        chunk.data.resize(size);
        in.read(chunk.data.data(), size);

        // Push chunk into PackrFile (uncompressed)
        file.add_chunk(chunk);
    }
}