#include "../algorithms.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <N> <dtype> <cutoff> <threads> <run>\n";
        return 1;
    }

#if !SEMINAR_HAS_TASKFLOW
    std::cerr << "Taskflow support is not available in this build\n";
    return 1;
#else
    size_t n = std::stoul(argv[1]);
    std::string dtype = argv[2];
    size_t cutoff = std::stoul(argv[3]);
    int threads = std::stoi(argv[4]);
    int run_id = std::stoi(argv[5]);
    uint64_t seed = 42;

    std::string input = std::to_string(n) + "_" + dtype + "_cutoff" + std::to_string(cutoff);
    const char* data_file = std::getenv("DATA_FILE");

    if (dtype == "u32") {
        auto data = data_file ? load_data_u32(data_file) : generate_u32(n, seed);
        uint64_t expected_sum = checksum_sum_u32(data);
        Timer timer;
        seminar::mergesort_taskflow(data, cutoff, static_cast<unsigned>(threads));
        double elapsed = timer.elapsed_secs();
        if (!is_sorted(data)) {
            std::cerr << "mergesort not sorted\n";
            return 1;
        }
        uint64_t actual_sum = checksum_sum_u32(data);
        if (actual_sum != expected_sum) {
            std::cerr << "checksum mismatch\n";
            return 1;
        }
        emit_csv("mergesort", input, "cpp_taskflow", threads, run_id, elapsed, actual_sum);
    } else if (dtype == "String") {
        auto data = data_file ? load_data_strings(data_file) : generate_strings(n, seed);
        uint64_t expected_sum = checksum_sum_strings(data);
        Timer timer;
        seminar::mergesort_taskflow(data, cutoff, static_cast<unsigned>(threads));
        double elapsed = timer.elapsed_secs();
        if (!is_sorted(data)) {
            std::cerr << "mergesort not sorted\n";
            return 1;
        }
        uint64_t actual_sum = checksum_sum_strings(data);
        if (actual_sum != expected_sum) {
            std::cerr << "checksum mismatch\n";
            return 1;
        }
        emit_csv("mergesort", input, "cpp_taskflow", threads, run_id, elapsed, actual_sum);
    } else {
        std::cerr << "unknown dtype: " << dtype << "\n";
        return 1;
    }

    return 0;
#endif
}
