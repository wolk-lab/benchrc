use crate::benchmarks::common::{emit_csv, load_graph, Graph, Timer};
use rayon::prelude::*;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::Mutex;

pub fn bfs(graph: &Graph, source: u32) -> u64 {
    let n = graph.num_nodes;
    let visited: Vec<AtomicBool> = (0..n).map(|_| AtomicBool::new(false)).collect();
    let mut frontier = Vec::new();
    let mut visited_count = 0u64;

    visited[source as usize].store(true, Ordering::Relaxed);
    frontier.push(source);
    visited_count += 1;

    while !frontier.is_empty() {
        let next_frontier = Mutex::new(Vec::new());

        frontier.par_iter().for_each(|&node| {
            let start = graph.offsets[node as usize];
            let end = graph.offsets[node as usize + 1];
            let mut local_next = Vec::new();
            for &neighbor in &graph.edges[start..end] {
                if !visited[neighbor as usize].load(Ordering::Relaxed)
                    && !visited[neighbor as usize].swap(true, Ordering::Acquire)
                {
                    local_next.push(neighbor);
                }
            }
            if !local_next.is_empty() {
                let mut guard = next_frontier.lock().unwrap();
                guard.extend(local_next);
            }
        });

        frontier = next_frontier.into_inner().unwrap();
        visited_count += frontier.len() as u64;
    }

    visited_count
}

pub fn run(input: &str, threads: usize, run_id: usize) {
    let pool = rayon::ThreadPoolBuilder::new()
        .num_threads(threads)
        .build()
        .unwrap();

    pool.install(|| {
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

        emit_csv(
            "bfs",
            input,
            "rust_rayon",
            threads,
            run_id,
            elapsed,
            visited_count,
        );
    });
}
