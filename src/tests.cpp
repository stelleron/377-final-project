#include <iostream>
#include "packr.hpp"

int main() {
    std::cout << "Hello World!" << std::endl;

    std::string in_path = "./";
    std::string out_path = "test/file.packr";

    Packr::compress(in_path, out_path);
}