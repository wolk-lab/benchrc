#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

static void BM_HistogramSeq_1M(benchmark::State& state) {
    const auto& data = histogram_uniform_1m();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_sequential(data, kHistogramBuckets);
        benchmark::DoNotOptimize(checksum_ordered_u64(result));
        benchmark::ClobberMemory();
    }
}

static void BM_HistogramOpenMP_1M(benchmark::State& state) {
    const auto& data = histogram_uniform_1m();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_openmp(data, kHistogramBuckets, threads);
        benchmark::DoNotOptimize(checksum_ordered_u64(result));
        benchmark::ClobberMemory();
    }
}

static void BM_HistogramTaskflow_1M(benchmark::State& state) {
    const auto& data = histogram_uniform_1m();
    unsigned threads = static_cast<unsigned>(state.range(0));
    tf::Executor executor(threads);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_taskflow(data, kHistogramBuckets, executor, threads);
        benchmark::DoNotOptimize(checksum_ordered_u64(result));
        benchmark::ClobberMemory();
    }
}

// --- 1B ---

static void BM_HistogramSeq_1B(benchmark::State& state) {
    const auto& data = histogram_uniform_1b();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_sequential(data, kHistogramBuckets);
        benchmark::DoNotOptimize(checksum_ordered_u64(result));
        benchmark::ClobberMemory();
    }
}

static void BM_HistogramOpenMP_1B(benchmark::State& state) {
    const auto& data = histogram_uniform_1b();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_openmp(data, kHistogramBuckets, threads);
        benchmark::DoNotOptimize(checksum_ordered_u64(result));
        benchmark::ClobberMemory();
    }
}

static void BM_HistogramTaskflow_1B(benchmark::State& state) {
    const auto& data = histogram_uniform_1b();
    unsigned threads = static_cast<unsigned>(state.range(0));
    tf::Executor executor(threads);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::histogram_taskflow(data, kHistogramBuckets, executor, threads);
        benchmark::DoNotOptimize(checksum_ordered_u64(result));
        benchmark::ClobberMemory();
    }
}

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_histogram();
    benchmark::Initialize(&argc, argv);

    configure_benchmark(benchmark::RegisterBenchmark("HistogramSeq/1M", &BM_HistogramSeq_1M));
    configure_benchmark(benchmark::RegisterBenchmark("HistogramSeq/1B", &BM_HistogramSeq_1B));
    add_thread_args(benchmark::RegisterBenchmark("HistogramOpenMP/1M", &BM_HistogramOpenMP_1M));
    add_thread_args(benchmark::RegisterBenchmark("HistogramOpenMP/1B", &BM_HistogramOpenMP_1B));
    add_thread_args(benchmark::RegisterBenchmark("HistogramTaskflow/1M", &BM_HistogramTaskflow_1M));
    add_thread_args(benchmark::RegisterBenchmark("HistogramTaskflow/1B", &BM_HistogramTaskflow_1B));

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
