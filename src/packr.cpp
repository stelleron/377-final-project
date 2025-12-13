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
        return;
    }
}

/* class PackrFile */
PackrFile::PackrFile(const std::string& path) {

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

    // Create a .packr file
    PackrFile file(out_path);
}