#include <iostream>
#include <chrono>
#include "packr.hpp"

auto time_run = [](auto&& fn, const std::string& label) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();

    double seconds =
        std::chrono::duration<double>(end - start).count();

    std::cout << label << ": " << seconds << " s\n";
};

int main() {
    std::string in_path = "test_data";

    std::string out_arc_path = "test/archive.packr";
    std::string out_comp_path = "test/compressed.packr";
    std::string out_unarch_path = "test/unarchived";
    std::string out_decomp_path = "test/decompressed";

    // Integrity test and space usage test
    Packr::archive(in_path, out_arc_path);
    Packr::compress(in_path, out_comp_path);
    Packr::unarchive(out_arc_path, out_unarch_path);
    Packr::decompress(out_comp_path, out_decomp_path);

    // Time test
    std::string seq_path = "test/out_seq.packr";
    std::string p5_path = "test/out_p5.packr";
    std::string p10_path = "test/out_p10.packr";

    time_run([&] {
        Packr::compress(in_path, seq_path);
    }, "Sequential");

    time_run([&] {
        Packr::compress_parallel(in_path, p5_path, 5);
    }, "Parallel (5 threads)");

    time_run([&] {
        Packr::compress_parallel(in_path, p10_path, 10);
    }, "Parallel (10 threads)");


    std::string out_decomp_seq_path = "test/decompressed_seq";
    std::string out_decomp_p5_path = "test/decompressed_p5";
    std::string out_decomp_p10_path = "test/decompressed_p10";

    time_run([&] {
        Packr::decompress(seq_path, out_decomp_seq_path);
    }, "Sequential");

    time_run([&] {
        Packr::decompress_parallel(p5_path, out_decomp_p5_path, 5);
    }, "Parallel (5 threads)");

    time_run([&] {
        Packr::decompress_parallel(p10_path, out_decomp_p10_path, 10);
    }, "Parallel (10 threads)");

}