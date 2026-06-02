#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

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

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_bfs();
    benchmark::Initialize(&argc, argv);

    add_thread_args(benchmark::RegisterBenchmark("BfsOpenMP/64K", &BM_BfsOpenMP_64K));
    add_thread_args(benchmark::RegisterBenchmark("BfsTaskflow/64K", &BM_BfsTaskflow_64K));

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
