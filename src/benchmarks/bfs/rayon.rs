use crate::benchmarks::common::Graph;
use rayon::prelude::*;
use std::sync::atomic::{AtomicBool, Ordering};

pub fn bfs(graph: &Graph, source: u32) -> u64 {
    let n = graph.num_nodes;
    let visited: Vec<AtomicBool> = (0..n).map(|_| AtomicBool::new(false)).collect();

    visited[source as usize].store(true, Ordering::Relaxed);
    let mut frontier = vec![source];
    let mut visited_count = 1u64;

    const CHUNK_SIZE: usize = 65536;

    while !frontier.is_empty() {
        let next: Vec<u32> = frontier
            .par_chunks(CHUNK_SIZE)
            .flat_map(|chunk| {
                let mut local = Vec::with_capacity(chunk.len() * 2);
                for &node in chunk {
                    let start = graph.offsets[node as usize];
                    let end = graph.offsets[node as usize + 1];
                    for &neighbor in &graph.edges[start..end] {
                        if !visited[neighbor as usize].swap(true, Ordering::Acquire) {
                            local.push(neighbor);
                        }
                    }
                }
                local
            })
            .collect();

        visited_count += next.len() as u64;
        frontier = next;
    }

    visited_count
}
