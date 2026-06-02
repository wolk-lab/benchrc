use crate::benchmarks::common::Graph;
use rayon::prelude::*;
use std::sync::Mutex;
use std::sync::atomic::{AtomicBool, Ordering};

pub fn bfs(graph: &Graph, source: u32) -> u64 {
    let visited: Vec<AtomicBool> = (0..graph.num_nodes)
        .map(|_| AtomicBool::new(false))
        .collect();
    let mut frontier = vec![source];
    let mut visited_count = 1u64;

    visited[source as usize].store(true, Ordering::Relaxed);

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
