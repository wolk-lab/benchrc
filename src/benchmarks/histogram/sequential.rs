use crate::benchmarks::common::{
    checksum_histogram, emit_csv, generate_uniform, generate_zipf, maybe_load_data_u64, Timer,
};

pub fn histogram(data: &[u64], buckets: usize) -> Vec<u64> {
    let mut hist = vec![0u64; buckets];
    for &value in data {
        hist[value as usize % buckets] += 1;
    }
    hist
}

pub fn run(input: &str, _threads: usize, run_id: usize) {
    let parts: Vec<&str> = input.split('_').collect();
    let n: usize = parts[0].parse().unwrap();
    let distribution = parts[1];
    let buckets: usize = parts[2].parse().unwrap();
    let seed = 42;

    let data = maybe_load_data_u64().unwrap_or_else(|| match distribution {
        "uniform" => generate_uniform(n, buckets, seed),
        "zipf" => generate_zipf(n, buckets, 1.5, seed),
        _ => panic!("unknown distribution: {}", distribution),
    });

    let timer = Timer::start();
    let result = histogram(&data, buckets);
    let elapsed = timer.elapsed_secs();

    let checksum = checksum_histogram(&result);
    assert_eq!(checksum, n as u64, "histogram checksum failed");

    emit_csv("histogram", input, "rust_seq", 1, run_id, elapsed, checksum);
}
