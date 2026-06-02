#include "../algorithms.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 6) {
        std::cerr << "Usage: " << argv[0] << " <N> <K> <radius> <threads> <run>\n";
        return 1;
    }

#if !SEMINAR_HAS_TASKFLOW
    std::cerr << "Taskflow support is not available in this build\n";
    return 1;
#else
    size_t n = std::stoul(argv[1]);
    size_t iterations = std::stoul(argv[2]);
    size_t radius = std::stoul(argv[3]);
    int threads = std::stoi(argv[4]);
    int run_id = std::stoi(argv[5]);
    uint64_t seed = 42;

    auto data = [&]() {
        if (const char* data_file = std::getenv("DATA_FILE")) {
            return load_data_f64(data_file);
        }
        return generate_f64(n, seed);
    }();

    Timer timer;
    auto result = seminar::stencil_taskflow(data, iterations, radius, static_cast<unsigned>(threads));
    double elapsed = timer.elapsed_secs();

    uint64_t checksum = checksum_sum_f64(result);
    std::string input = std::to_string(n) + "_k" + std::to_string(iterations) + "_r" + std::to_string(radius);
    emit_csv("stencil", input, "cpp_taskflow", threads, run_id, elapsed, checksum);
    return 0;
#endif
}
