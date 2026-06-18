#pragma once

#include "../algorithms.hpp"

#include <benchmark/benchmark.h>

#include <stdexcept>
#include <vector>

namespace seminar::benchmarks {

constexpr int kThreadCounts[] = {1, 2, 4, 8, 16, 32};
constexpr size_t kHistogramBuckets = 256;
constexpr size_t kMergesortCutoff = 1024;
constexpr size_t kStencilIterations = 10;
constexpr size_t kStencilRadius = 1;

inline benchmark::Benchmark* configure_benchmark(benchmark::Benchmark* bench) {
    return bench->UseRealTime()->MinWarmUpTime(10.0)->Repetitions(10)->DisplayAggregatesOnly(true);
}

inline void add_thread_args(benchmark::Benchmark* bench) {
    for (int threads : kThreadCounts) {
        bench->Arg(threads);
    }
    configure_benchmark(bench);
}

inline const std::vector<uint64_t>& histogram_uniform_1m() {
    static const auto data = load_u64_dataset("histogram_uniform_1m.u64le.bin", 1'000'000);
    return data;
}

inline const std::vector<uint64_t>& histogram_uniform_1b() {
    static const auto data = load_u64_dataset("histogram_uniform_1b.u64le.bin", 1'000'000'000);
    return data;
}

inline const std::vector<uint32_t>& mergesort_u32_1b() {
    static const auto data = load_u32_dataset("mergesort_u32_1b.u32le.bin", 1'000'000'000);
    return data;
}

inline const std::vector<double>& stencil_f64_1b() {
    static const auto data = load_f64_dataset("stencil_f64_1b.f64le.bin", 1'000'000'000);
    return data;
}

inline const Graph& bfs_graph_320m() {
    static const auto graph = load_graph_dataset("bfs_320m_tree.graph.bin");
    return graph;
}

inline void verify_histogram() {
    auto seq_result = seminar::histogram_sequential(histogram_uniform_1m(), kHistogramBuckets);
    if (checksum_histogram(seq_result) != histogram_uniform_1m().size()) {
        throw std::runtime_error("sequential histogram verification failed");
    }

    auto openmp_result = seminar::histogram_openmp(histogram_uniform_1m(), kHistogramBuckets, 1);
    if (checksum_histogram(openmp_result) != histogram_uniform_1m().size()) {
        throw std::runtime_error("OpenMP histogram verification failed");
    }

    tf::Executor executor(1);
    auto taskflow_result =
        seminar::histogram_taskflow(histogram_uniform_1m(), kHistogramBuckets, executor, 1);
    if (checksum_histogram(taskflow_result) != histogram_uniform_1m().size()) {
        throw std::runtime_error("Taskflow histogram verification failed");
    }
}

inline void verify_mergesort() {
    {
        auto values = mergesort_u32_1b();
        auto expected = checksum_sum_u32(values);
        seminar::mergesort_sequential(values, kMergesortCutoff);
        if (!is_sorted(values) || checksum_sum_u32(values) != expected) {
            throw std::runtime_error("sequential mergesort verification failed");
        }
    }

    {
        auto values = mergesort_u32_1b();
        auto expected = checksum_sum_u32(values);
        seminar::mergesort_openmp(values, kMergesortCutoff, 1);
        if (!is_sorted(values) || checksum_sum_u32(values) != expected) {
            throw std::runtime_error("OpenMP mergesort verification failed");
        }
    }

    {
        auto values = mergesort_u32_1b();
        auto expected = checksum_sum_u32(values);
        tf::Executor executor(1);
        seminar::mergesort_taskflow(values, kMergesortCutoff, executor);
        if (!is_sorted(values) || checksum_sum_u32(values) != expected) {
            throw std::runtime_error("Taskflow mergesort verification failed");
        }
    }
}

inline void verify_stencil() {
    auto seq_result = seminar::stencil_sequential(stencil_f64_1b(), kStencilIterations, kStencilRadius);
    benchmark::DoNotOptimize(seq_result.data());

    auto openmp_result =
        seminar::stencil_openmp(stencil_f64_1b(), kStencilIterations, kStencilRadius, 1);
    benchmark::DoNotOptimize(openmp_result.data());

    tf::Executor executor(1);
    auto taskflow_result =
        seminar::stencil_taskflow(stencil_f64_1b(), kStencilIterations, kStencilRadius, executor, 1);
    benchmark::DoNotOptimize(taskflow_result.data());
}

inline void verify_bfs() {
    auto seq_visited = seminar::bfs_sequential(bfs_graph_320m(), 0);
    if (seq_visited != bfs_graph_320m().num_nodes) {
        throw std::runtime_error("sequential BFS verification failed");
    }

    auto openmp_visited = seminar::bfs_openmp(bfs_graph_320m(), 0, 1);
    if (openmp_visited != bfs_graph_320m().num_nodes) {
        throw std::runtime_error("OpenMP BFS verification failed");
    }

    tf::Executor executor(1);
    auto taskflow_visited = seminar::bfs_taskflow(bfs_graph_320m(), 0, executor, 1);
    if (taskflow_visited != bfs_graph_320m().num_nodes) {
        throw std::runtime_error("Taskflow BFS verification failed");
    }
}

} // namespace seminar::benchmarks
