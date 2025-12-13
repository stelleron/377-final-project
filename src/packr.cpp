#include "packr.hpp"
#include <sys/stat.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>

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
    stat(path.c_str(), &buffer);
    if (buffer.st_mode & S_IFREG )
        return FILE_PATH;
    else if (buffer.st_mode & S_IFDIR )
        return DIR_PATH;
    else
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
        std::cerr << "Error: Given invalid path! Path must be an existing directory" << std::endl;
        exit(1);
    }
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
            chunks[x].data = new char[chunks[x].header.comp_size];
            file.read(chunks[x].data, chunks[x].header.comp_size);
        }

        std::cout << "Loaded all chunks!" << std::endl;
    }

    // Close file - we'll open it later for writing all our data
    file.close();
}


PackrFile::~PackrFile() {
    file.close();
    if (file.is_open()) {
        std::cerr << "ERROR: Unable to close file system." << std::endl;
        exit(1);
    }
}

/* class Packr */
/* =========== */
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

}