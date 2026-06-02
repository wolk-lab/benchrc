#include "../algorithms.hpp"

#include <benchmark/benchmark.h>

#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace {

constexpr int kThreadCounts[] = {1, 2, 4, 8};
constexpr size_t kHistogramBuckets = 256;
constexpr size_t kMergesortCutoff = 1024;
constexpr size_t kStencilIterations = 10;
constexpr size_t kStencilRadius = 1;

void add_thread_args(benchmark::internal::Benchmark* bench) {
    for (int threads : kThreadCounts) {
        bench->Arg(threads);
    }
    bench->UseRealTime()->MinWarmUpTime(1.0);
}

const std::vector<uint64_t>& histogram_uniform_1m() {
    static const auto data = generate_uniform(1'000'000, kHistogramBuckets, 42);
    return data;
}

const std::vector<uint64_t>& histogram_uniform_10m() {
    static const auto data = generate_uniform(10'000'000, kHistogramBuckets, 42);
    return data;
}

const std::vector<uint32_t>& mergesort_u32_1m() {
    static const auto data = generate_u32(1'000'000, 42);
    return data;
}

const std::vector<double>& stencil_f64_1m() {
    static const auto data = generate_f64(1'000'000, 42);
    return data;
}

const Graph& bfs_graph_64k() {
    static const auto graph = seminar::benchmark_graph(65'536, 4);
    return graph;
}

void verify_bench_inputs() {
#if SEMINAR_HAS_OPENMP
    {
        auto result = seminar::histogram_openmp(histogram_uniform_1m(), kHistogramBuckets, 1);
        if (checksum_histogram(result) != histogram_uniform_1m().size()) {
            throw std::runtime_error("OpenMP histogram verification failed");
        }
    }
    {
        auto values = mergesort_u32_1m();
        auto expected = checksum_sum_u32(values);
        seminar::mergesort_openmp(values, kMergesortCutoff, 1);
        if (!is_sorted(values) || checksum_sum_u32(values) != expected) {
            throw std::runtime_error("OpenMP mergesort verification failed");
        }
    }
    {
        auto result = seminar::stencil_openmp(stencil_f64_1m(), kStencilIterations, kStencilRadius, 1);
        benchmark::DoNotOptimize(result.data());
    }
    {
        auto visited = seminar::bfs_openmp(bfs_graph_64k(), 0, 1);
        if (visited != bfs_graph_64k().num_nodes) {
            throw std::runtime_error("OpenMP BFS verification failed");
        }
    }
#endif

#if SEMINAR_HAS_TASKFLOW
    {
        auto result = seminar::histogram_taskflow(histogram_uniform_1m(), kHistogramBuckets, 1, 4);
        if (checksum_histogram(result) != histogram_uniform_1m().size()) {
            throw std::runtime_error("Taskflow histogram verification failed");
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
    {
        auto result = seminar::stencil_taskflow(stencil_f64_1m(), kStencilIterations, kStencilRadius, 1);
        benchmark::DoNotOptimize(result.data());
    }
    {
        auto visited = seminar::bfs_taskflow(bfs_graph_64k(), 0, 1);
        if (visited != bfs_graph_64k().num_nodes) {
            throw std::runtime_error("Taskflow BFS verification failed");
        }
    }
#endif
}

#if SEMINAR_HAS_OPENMP
static void BM_HistogramOpenMP_1M(benchmark::State& state) {
    const auto& data = histogram_uniform_1m();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_openmp(data, kHistogramBuckets, threads);
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
}

static void BM_HistogramOpenMP_10M(benchmark::State& state) {
    const auto& data = histogram_uniform_10m();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_openmp(data, kHistogramBuckets, threads);
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
}

static void BM_MergesortOpenMP_1M(benchmark::State& state) {
    const auto& source = mergesort_u32_1m();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(source.size()));

    for (auto _ : state) {
        auto values = source;
        seminar::mergesort_openmp(values, kMergesortCutoff, threads);
        benchmark::DoNotOptimize(values.data());
        benchmark::ClobberMemory();
    }
}

static void BM_StencilOpenMP_1M(benchmark::State& state) {
    const auto& data = stencil_f64_1m();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::stencil_openmp(data, kStencilIterations, kStencilRadius, threads);
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
}

static void BM_BfsOpenMP_64K(benchmark::State& state) {
    const auto& graph = bfs_graph_64k();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(graph.num_nodes));

    for (auto _ : state) {
        auto visited = seminar::bfs_openmp(graph, 0, threads);
        benchmark::DoNotOptimize(visited);
        benchmark::ClobberMemory();
    }
}
#endif

#if SEMINAR_HAS_TASKFLOW
static void BM_HistogramTaskflow_1M(benchmark::State& state) {
    const auto& data = histogram_uniform_1m();
    unsigned threads = static_cast<unsigned>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_taskflow(data, kHistogramBuckets, threads, threads * 4);
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
}

static void BM_HistogramTaskflow_10M(benchmark::State& state) {
    const auto& data = histogram_uniform_10m();
    unsigned threads = static_cast<unsigned>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_taskflow(data, kHistogramBuckets, threads, threads * 4);
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
}

static void BM_MergesortTaskflow_1M(benchmark::State& state) {
    const auto& source = mergesort_u32_1m();
    unsigned threads = static_cast<unsigned>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(source.size()));

    for (auto _ : state) {
        auto values = source;
        seminar::mergesort_taskflow(values, kMergesortCutoff, threads);
        benchmark::DoNotOptimize(values.data());
        benchmark::ClobberMemory();
    }
}

static void BM_StencilTaskflow_1M(benchmark::State& state) {
    const auto& data = stencil_f64_1m();
    unsigned threads = static_cast<unsigned>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::stencil_taskflow(data, kStencilIterations, kStencilRadius, threads);
        benchmark::DoNotOptimize(result.data());
        benchmark::ClobberMemory();
    }
}

static void BM_BfsTaskflow_64K(benchmark::State& state) {
    const auto& graph = bfs_graph_64k();
    unsigned threads = static_cast<unsigned>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(graph.num_nodes));

    for (auto _ : state) {
        auto visited = seminar::bfs_taskflow(graph, 0, threads);
        benchmark::DoNotOptimize(visited);
        benchmark::ClobberMemory();
    }
}
#endif

}  // namespace

int main(int argc, char** argv) {
    verify_bench_inputs();
    benchmark::Initialize(&argc, argv);

#if SEMINAR_HAS_OPENMP
    add_thread_args(benchmark::RegisterBenchmark("HistogramOpenMP/1M", &BM_HistogramOpenMP_1M));
    add_thread_args(benchmark::RegisterBenchmark("HistogramOpenMP/10M", &BM_HistogramOpenMP_10M));
    add_thread_args(benchmark::RegisterBenchmark("MergesortOpenMP/1M", &BM_MergesortOpenMP_1M));
    add_thread_args(benchmark::RegisterBenchmark("StencilOpenMP/1M", &BM_StencilOpenMP_1M));
    add_thread_args(benchmark::RegisterBenchmark("BfsOpenMP/64K", &BM_BfsOpenMP_64K));
#endif

#if SEMINAR_HAS_TASKFLOW
    add_thread_args(benchmark::RegisterBenchmark("HistogramTaskflow/1M", &BM_HistogramTaskflow_1M));
    add_thread_args(benchmark::RegisterBenchmark("HistogramTaskflow/10M", &BM_HistogramTaskflow_10M));
    add_thread_args(benchmark::RegisterBenchmark("MergesortTaskflow/1M", &BM_MergesortTaskflow_1M));
    add_thread_args(benchmark::RegisterBenchmark("StencilTaskflow/1M", &BM_StencilTaskflow_1M));
    add_thread_args(benchmark::RegisterBenchmark("BfsTaskflow/64K", &BM_BfsTaskflow_64K));
#endif

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
