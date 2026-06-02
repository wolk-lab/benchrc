#include "../algorithms.hpp"

#include <iostream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <N> <buckets> <distribution> <threads> <run>\n";
        return 1;
    }

#if !SEMINAR_HAS_TASKFLOW
    std::cerr << "Taskflow support is not available in this build\n";
    return 1;
#else
    size_t n = std::stoul(argv[1]);
    uint64_t buckets = std::stoul(argv[2]);
    std::string distribution = argv[3];
    int threads = std::stoi(argv[4]);
    int run_id = std::stoi(argv[5]);
    uint64_t seed = 42;

    std::vector<uint64_t> data;
    if (const char* data_file = std::getenv("DATA_FILE")) {
        data = load_data_u64(data_file);
    } else if (distribution == "uniform") {
        data = generate_uniform(n, buckets, seed);
    } else if (distribution == "zipf") {
        data = generate_zipf(n, buckets, 1.5, seed);
    } else {
        std::cerr << "unknown distribution: " << distribution << "\n";
        return 1;
    }

    Timer timer;
    auto result = seminar::histogram_taskflow(data, buckets, static_cast<unsigned>(threads),
                                              static_cast<unsigned>(threads * 4));
    double elapsed = timer.elapsed_secs();

    uint64_t checksum = checksum_histogram(result);
    if (checksum != n) {
        std::cerr << "histogram checksum failed: " << checksum << " != " << n << "\n";
        return 1;
    }

    std::string input = std::to_string(n) + "_" + distribution + "_" + std::to_string(buckets);
    emit_csv("histogram", input, "cpp_taskflow", threads, run_id, elapsed, checksum);
    return 0;
#endif
}
