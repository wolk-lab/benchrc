#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

static void BM_MergesortSeq_1B(benchmark::State& state) {
    const auto& source = mergesort_u32_1b();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(source.size()));

    for (auto _ : state) {
        auto values = source;
        seminar::mergesort_sequential(values, kMergesortCutoff);
        benchmark::DoNotOptimize(is_sorted(values));
        benchmark::DoNotOptimize(checksum_ordered_u32(values));
        benchmark::ClobberMemory();
    }
}

static void BM_MergesortOpenMP_1B(benchmark::State& state) {
    const auto& source = mergesort_u32_1b();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(source.size()));

    for (auto _ : state) {
        auto values = source;
        seminar::mergesort_openmp(values, kMergesortCutoff, threads);
        benchmark::DoNotOptimize(is_sorted(values));
        benchmark::DoNotOptimize(checksum_ordered_u32(values));
        benchmark::ClobberMemory();
    }
}

static void BM_MergesortTaskflow_1B(benchmark::State& state) {
    const auto& source = mergesort_u32_1b();
    unsigned threads = static_cast<unsigned>(state.range(0));
    tf::Executor executor(threads);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(source.size()));

    for (auto _ : state) {
        auto values = source;
        seminar::mergesort_taskflow(values, kMergesortCutoff, executor);
        benchmark::DoNotOptimize(is_sorted(values));
        benchmark::DoNotOptimize(checksum_ordered_u32(values));
        benchmark::ClobberMemory();
    }
}

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_mergesort();
    benchmark::Initialize(&argc, argv);

    configure_benchmark(benchmark::RegisterBenchmark("MergesortSeq/1B", &BM_MergesortSeq_1B));
    add_thread_args(benchmark::RegisterBenchmark("MergesortOpenMP/1B", &BM_MergesortOpenMP_1B));
    add_thread_args(benchmark::RegisterBenchmark("MergesortTaskflow/1B", &BM_MergesortTaskflow_1B));

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
