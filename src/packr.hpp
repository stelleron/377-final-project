#ifndef PACKR_HPP
    #define PACKR_HPP
    #include <string>
    #include <fstream>

    /* Macros */
    // We use the first 8 chunks for storing the status of each block
    // We use the next 8 chunks to store i-nodes
    #define CHUNK_LIMIT 32768 // Total number of chunks
    #define CHUNK_SIZE 4096 // Each chunk is 8 KiB
    #define FREE_CHUNKS_LIST_SIZE CHUNK_LIMIT/CHUNK_SIZE

    #define INODE_SIZE 64 // Size of each i-node for calculations
    #define INODE_LIST_SIZE 8 // Size in chunks
    #define NUM_FILES CHUNK_SIZE/INODE_SIZE * INODE_LIST_SIZE // Number of files we can have

    // Packr INode must be 64 bytes
    // Meaning each file is max 72 KiB
    struct PackrINode {
        char name[16]; // 4*4 = 16 bytes
        char ext[4]; // 4 bytes
        int num_chunks; // 4 bytes
        int chunk_pointers[9]; // 4*9 bytes = 36 bytes
        int used; // 4 bytes
    };

    // Implements an in-memory class for .packr files
    class PackrFile {
        private:
            std::fstream file;
        public:
            PackrFile(const std::string& path); // Load an existing .packr file or create a new one
            ~PackrFile();
    };

    // Implements Packr's functions
    class Packr {
        public:
            // Single thread
            static void compress(std::string& in_path, std::string& out_path);
            static void decompress(std::string& in_path, std::string& out_path);

            // Multiple threads
            static void compress_parallel(std::string& in_path, std::string& out_path, int num_threads);
    };
#endif