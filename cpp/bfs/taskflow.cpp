#include "../algorithms.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    if (argc < 4) {
        std::cerr << "Usage: " << argv[0] << " <graph_path> <threads> <run>\n";
        return 1;
    }

#if !SEMINAR_HAS_TASKFLOW
    std::cerr << "Taskflow support is not available in this build\n";
    return 1;
#else
    std::string graph_path = argv[1];
    int threads = std::stoi(argv[2]);
    int run_id = std::stoi(argv[3]);

    auto graph = load_graph(graph_path);

    Timer timer;
    uint64_t visited_count = seminar::bfs_taskflow(graph, 0, static_cast<unsigned>(threads));
    double elapsed = timer.elapsed_secs();

    if (visited_count != graph.num_nodes) {
        std::cerr << "BFS visited " << visited_count << " of " << graph.num_nodes << " nodes\n";
        return 1;
    }

    emit_csv("bfs", graph_path, "cpp_taskflow", threads, run_id, elapsed, visited_count);
    return 0;
#endif
}
