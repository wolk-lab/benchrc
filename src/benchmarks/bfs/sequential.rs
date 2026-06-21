use crate::benchmarks::common::Graph;

pub fn bfs(graph: &Graph, source: u32) -> u64 {
    let mut visited = vec![false; graph.num_nodes];
    let mut frontier = vec![source];
    let mut visited_count = 1u64;

    visited[source as usize] = true;

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
