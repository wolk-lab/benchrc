#pragma once

#include "../algorithms.hpp"

#include <benchmark/benchmark.h>

#include <stdexcept>
#include <vector>

namespace seminar::benchmarks {

constexpr int kThreadCounts[] = {1, 2, 4, 8};
constexpr size_t kHistogramBuckets = 256;
constexpr size_t kMergesortCutoff = 1024;
constexpr size_t kStencilIterations = 10;
constexpr size_t kStencilRadius = 1;

inline void add_thread_args(benchmark::internal::Benchmark* bench) {
    for (int threads : kThreadCounts) {
        bench->Arg(threads);
    }
    bench->UseRealTime()->MinWarmUpTime(3.0)->Repetitions(30)->DisplayAggregatesOnly(true);
}

inline const std::vector<uint64_t>& histogram_uniform_1m() {
    static const auto data = load_u64_dataset("histogram_uniform_1m.u64le.bin", 1'000'000);
    return data;
}

inline const std::vector<uint64_t>& histogram_uniform_10m() {
    static const auto data = load_u64_dataset("histogram_uniform_10m.u64le.bin", 10'000'000);
    return data;
}

inline const std::vector<uint32_t>& mergesort_u32_1m() {
    static const auto data = load_u32_dataset("mergesort_u32_1m.u32le.bin", 1'000'000);
    return data;
}

inline const std::vector<double>& stencil_f64_1m() {
    static const auto data = load_f64_dataset("stencil_f64_1m.f64le.bin", 1'000'000);
    return data;
}

inline const Graph& bfs_graph_64k() {
    static const auto graph = load_graph_dataset("bfs_64k_fanout4.graph.bin");
    return graph;
}

inline void verify_histogram() {
    auto openmp_result = seminar::histogram_openmp(histogram_uniform_1m(), kHistogramBuckets, 1);
    if (checksum_histogram(openmp_result) != histogram_uniform_1m().size()) {
        throw std::runtime_error("OpenMP histogram verification failed");
    }

    auto taskflow_result =
        seminar::histogram_taskflow(histogram_uniform_1m(), kHistogramBuckets, 1, 4);
    if (checksum_histogram(taskflow_result) != histogram_uniform_1m().size()) {
        throw std::runtime_error("Taskflow histogram verification failed");
    }
}

inline void verify_mergesort() {
    {
        auto values = mergesort_u32_1m();
        auto expected = checksum_sum_u32(values);
        seminar::mergesort_openmp(values, kMergesortCutoff, 1);
        if (!is_sorted(values) || checksum_sum_u32(values) != expected) {
            throw std::runtime_error("OpenMP mergesort verification failed");
        }
    }

    {
        auto values = mergesort_u32_1m();
        auto expected = checksum_sum_u32(values);
        seminar::mergesort_taskflow(values, kMergesortCutoff, 1);
        if (!is_sorted(values) || checksum_sum_u32(values) != expected) {
            throw std::runtime_error("Taskflow mergesort verification failed");
        }
    }
}

inline void verify_stencil() {
    auto openmp_result =
        seminar::stencil_openmp(stencil_f64_1m(), kStencilIterations, kStencilRadius, 1);
    benchmark::DoNotOptimize(openmp_result.data());

    auto taskflow_result =
        seminar::stencil_taskflow(stencil_f64_1m(), kStencilIterations, kStencilRadius, 1);
    benchmark::DoNotOptimize(taskflow_result.data());
}

inline void verify_bfs() {
    auto openmp_visited = seminar::bfs_openmp(bfs_graph_64k(), 0, 1);
    if (openmp_visited != bfs_graph_64k().num_nodes) {
        throw std::runtime_error("OpenMP BFS verification failed");
    }

    auto taskflow_visited = seminar::bfs_taskflow(bfs_graph_64k(), 0, 1);
    if (taskflow_visited != bfs_graph_64k().num_nodes) {
        throw std::runtime_error("Taskflow BFS verification failed");
    }
}

} // namespace seminar::benchmarks
