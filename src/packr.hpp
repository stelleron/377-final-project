#ifndef PACKR_HPP
    #define PACKR_HPP
    #include <string>
    #include <fstream>

    #define PACKR_VERSION "1.0.0"

    // Struct for the file header
    struct FileHeader {
        char version[16];
        uint32_t chunk_count;
    };

    // Struct for every data header
    struct DataHeader {
        char alias[256];
        uint32_t base_size;
        uint32_t comp_size;
    };

    // Struct to store chunks of data
    struct DataChunk {
        DataHeader header;
        std::vector<char> data;
    };

    // Implements an in-memory class for .packr files
    class PackrFile {
        private:
            std::fstream file;
            std::string file_path;
            FileHeader header;
            std::vector<DataChunk> chunks;
        public:
            PackrFile(const std::string& path, bool new_file); // Load an existing .packr file or create a new one
            ~PackrFile();

            void add_chunk(DataChunk& chunk); // Uncompressed chunk
            void add_compressed_chunk(DataChunk& chunk); // Pre-compressed chunk
            void flush(); // Write all chunks to file

            const std::vector<DataChunk>& get_chunks() const { return chunks; }
    };

    // Implements Packr's functions
    class Packr {
        public:
            // Single thread
            static void archive(std::string& in_path, std::string& out_path); // Simply bundle files, don't decompress
            static void unarchive(std::string& in_path, std::string& out_path);

            static void compress(std::string& in_path, std::string& out_path);
            static void decompress(std::string& in_path, std::string& out_path);

            // Multiple threads
            static void compress_parallel(std::string& in_path, std::string& out_path, int num_threads);
            static void decompress_parallel(std::string& in_path, std::string& out_path, int num_threads);
    };
#endif