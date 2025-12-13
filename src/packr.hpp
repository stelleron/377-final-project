#ifndef PACKR_HPP
    #define PACKR_HPP
    #include <string>

    // Implements Packr's functions
    class Packr {
        public:
            static void compress(std::string& in_path, std::string& out_path);
            static void decompress(std::string& in_path, std::string& out_path);
    };
#endif