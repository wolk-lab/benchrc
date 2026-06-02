use crate::benchmarks::common::{emit_csv, load_graph, Graph, Timer};

pub fn bfs(graph: &Graph, source: u32) -> u64 {
    let n = graph.num_nodes;
    let mut visited = vec![false; n];
    let mut frontier = Vec::new();
    let mut visited_count = 0u64;

    visited[source as usize] = true;
    frontier.push(source);
    visited_count += 1;

    while !frontier.is_empty() {
        let mut next_frontier = Vec::new();
        for &node in &frontier {
            let start = graph.offsets[node as usize];
            let end = graph.offsets[node as usize + 1];
            for &neighbor in &graph.edges[start..end] {
                if !visited[neighbor as usize] {
                    visited[neighbor as usize] = true;
                    next_frontier.push(neighbor);
                    visited_count += 1;
                }
            }
        }
        frontier = next_frontier;
    }

    visited_count
}

pub fn run(input: &str, _threads: usize, run_id: usize) {
    let graph = load_graph(input);
    let source = 0u32;

    let timer = Timer::start();
    let visited_count = bfs(&graph, source);
    let elapsed = timer.elapsed_secs();

    let expected_nodes = graph.num_nodes as u64;
    assert_eq!(
        visited_count, expected_nodes,
        "BFS visited {} of {} nodes (graph may not be fully connected from source {})",
        visited_count, expected_nodes, source
    );

    emit_csv("bfs", input, "rust_seq", 1, run_id, elapsed, visited_count);
}
