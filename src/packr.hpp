#ifndef PACKR_HPP
    #define PACKR_HPP
    #include <string>

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