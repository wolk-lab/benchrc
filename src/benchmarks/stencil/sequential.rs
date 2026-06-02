use crate::benchmarks::common::{
    checksum_sum_f64, emit_csv, generate_f64, maybe_load_data_f64, Timer,
};

pub fn stencil(values: &[f64], iterations: usize, radius: usize) -> Vec<f64> {
    let n = values.len();
    let mut current = values.to_vec();
    let mut next = vec![0.0; n];

    for _ in 0..iterations {
        for (index, item) in next.iter_mut().enumerate() {
            let start = index.saturating_sub(radius);
            let end = (index + radius + 1).min(n);
            let count = end - start;
            let sum: f64 = current[start..end].iter().sum();
            *item = sum / count as f64;
        }
        std::mem::swap(&mut current, &mut next);
    }

    current
}

pub fn run(input: &str, _threads: usize, run_id: usize) {
    let parts: Vec<&str> = input.split('_').collect();
    let n: usize = parts[0].parse().unwrap();
    let iterations: usize = parts[1].strip_prefix("k").unwrap().parse().unwrap();
    let radius: usize = parts[2].strip_prefix("r").unwrap().parse().unwrap();
    let seed = 42;

    let data = maybe_load_data_f64().unwrap_or_else(|| generate_f64(n, seed));

    let timer = Timer::start();
    let result = stencil(&data, iterations, radius);
    let elapsed = timer.elapsed_secs();

    let checksum = checksum_sum_f64(&result);
    emit_csv("stencil", input, "rust_seq", 1, run_id, elapsed, checksum);
}
