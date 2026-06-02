#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

#if SEMINAR_HAS_OPENMP
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
#endif

#if SEMINAR_HAS_TASKFLOW
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
#endif

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_stencil();
    benchmark::Initialize(&argc, argv);

#if SEMINAR_HAS_OPENMP
    add_thread_args(benchmark::RegisterBenchmark("StencilOpenMP/1M", &BM_StencilOpenMP_1M));
#endif

#if SEMINAR_HAS_TASKFLOW
    add_thread_args(benchmark::RegisterBenchmark("StencilTaskflow/1M", &BM_StencilTaskflow_1M));
#endif

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
