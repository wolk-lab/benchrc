#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

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

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_mergesort();
    benchmark::Initialize(&argc, argv);

    add_thread_args(benchmark::RegisterBenchmark("MergesortOpenMP/1M", &BM_MergesortOpenMP_1M));
    add_thread_args(benchmark::RegisterBenchmark("MergesortTaskflow/1M", &BM_MergesortTaskflow_1M));

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
