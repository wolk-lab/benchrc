#include "common.hpp"

namespace {

using namespace seminar::benchmarks;

static void BM_BfsSeq_6M(benchmark::State& state) {
    const auto& graph = bfs_graph_6m();
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(graph.num_nodes));

    for (auto _ : state) {
        auto visited = seminar::bfs_sequential(graph, 0);
        benchmark::DoNotOptimize(visited);
        benchmark::ClobberMemory();
    }
}

static void BM_BfsOpenMP_6M(benchmark::State& state) {
    const auto& graph = bfs_graph_6m();
    int threads = static_cast<int>(state.range(0));
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(graph.num_nodes));

    for (auto _ : state) {
        auto visited = seminar::bfs_openmp(graph, 0, threads);
        benchmark::DoNotOptimize(visited);
        benchmark::ClobberMemory();
    }
}

static void BM_BfsTaskflow_6M(benchmark::State& state) {
    const auto& graph = bfs_graph_6m();
    unsigned threads = static_cast<unsigned>(state.range(0));
    tf::Executor executor(threads);
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(graph.num_nodes));

    for (auto _ : state) {
        auto visited = seminar::bfs_taskflow(graph, 0, executor, threads);
        benchmark::DoNotOptimize(visited);
        benchmark::ClobberMemory();
    }
}

} // namespace

int main(int argc, char** argv) {
    seminar::benchmarks::verify_bfs();
    benchmark::Initialize(&argc, argv);

    configure_benchmark(benchmark::RegisterBenchmark("BfsSeq/6M", &BM_BfsSeq_6M));
    add_thread_args(benchmark::RegisterBenchmark("BfsOpenMP/6M", &BM_BfsOpenMP_6M));
    add_thread_args(benchmark::RegisterBenchmark("BfsTaskflow/6M", &BM_BfsTaskflow_6M));

    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
