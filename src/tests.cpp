#include <iostream>
#include "packr.hpp"

int main() {
    std::cout << "Hello World!" << std::endl;

    std::string in_path = "test_data";
    std::string out_arc_path = "test/archive.packr";
    std::string out_comp_path = "test/compressed.packr";
    std::string out_unarch_path = "test/unarchived";
    std::string out_decomp_path = "test/decompressed";

    Packr::archive(in_path, out_arc_path);
    Packr::compress(in_path, out_comp_path);
    Packr::unarchive(out_arc_path, out_unarch_path);
    Packr::decompress(out_comp_path, out_decomp_path);
}