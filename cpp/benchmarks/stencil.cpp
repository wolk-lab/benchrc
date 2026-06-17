#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

static void BM_StencilSeq_1B(benchmark::State& state) {
    const auto& data = stencil_f64_1b();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::stencil_sequential(data, kStencilIterations, kStencilRadius);
        benchmark::DoNotOptimize(checksum_sum_f64(result));
        benchmark::ClobberMemory();
    }
}

static void BM_StencilOpenMP_1B(benchmark::State& state) {
    const auto& data = stencil_f64_1b();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::stencil_openmp(data, kStencilIterations, kStencilRadius, threads);
        benchmark::DoNotOptimize(checksum_sum_f64(result));
        benchmark::ClobberMemory();
    }
}

static void BM_StencilTaskflow_1B(benchmark::State& state) {
    const auto& data = stencil_f64_1b();
    unsigned threads = static_cast<unsigned>(state.range(0));
    tf::Executor executor(threads);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(data.size()));

    for (auto _ : state) {
        auto result = seminar::stencil_taskflow(data, kStencilIterations, kStencilRadius, executor, threads);
        benchmark::DoNotOptimize(checksum_sum_f64(result));
        benchmark::ClobberMemory();
    }
}

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_stencil();
    benchmark::Initialize(&argc, argv);

    configure_benchmark(benchmark::RegisterBenchmark("StencilSeq/1B", &BM_StencilSeq_1B));
    add_thread_args(benchmark::RegisterBenchmark("StencilOpenMP/1B", &BM_StencilOpenMP_1B));
    add_thread_args(benchmark::RegisterBenchmark("StencilTaskflow/1B", &BM_StencilTaskflow_1B));

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
