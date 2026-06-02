use rand::RngExt;
use rand::SeedableRng;
use rand_distr::{Distribution, Zipf};

const FNV_OFFSET: u64 = 0xcbf29ce484222325;
const FNV_PRIME: u64 = 0x100000001b3;

pub struct Graph {
    pub offsets: Vec<usize>,
    pub edges: Vec<u32>,
    pub num_nodes: usize,
}

pub fn generate_uniform(n: usize, buckets: usize, seed: u64) -> Vec<u64> {
    let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
    (0..n)
        .map(|_| rng.random_range(0..buckets as u64))
        .collect()
}

pub fn generate_zipf(n: usize, buckets: usize, alpha: f64, seed: u64) -> Vec<u64> {
    let zipf = Zipf::new(buckets as f64, alpha).unwrap();
    let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
    (0..n).map(|_| zipf.sample(&mut rng) as u64 - 1).collect()
}

pub fn generate_u32(n: usize, seed: u64) -> Vec<u32> {
    let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
    (0..n).map(|_| rng.random()).collect()
}

pub fn generate_f64(n: usize, seed: u64) -> Vec<f64> {
    let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
    (0..n).map(|_| rng.random()).collect()
}

fn mix64(mut value: u64) -> u64 {
    value = value.wrapping_add(0x9e3779b97f4a7c15);
    value = (value ^ (value >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)).wrapping_mul(0x94d049bb133111eb);
    value ^ (value >> 31)
}

pub fn checksum_histogram(hist: &[u64]) -> u64 {
    hist.iter().sum()
}

pub fn checksum_sum_u32(data: &[u32]) -> u64 {
    data.iter()
        .fold(0u64, |sum, &value| sum.wrapping_add(mix64(value as u64)))
}

pub fn checksum_sum_f64(data: &[f64]) -> u64 {
    data.iter()
        .enumerate()
        .fold(FNV_OFFSET, |hash, (index, &value)| {
            (hash ^ mix64(value.to_bits() ^ index as u64)).wrapping_mul(FNV_PRIME)
        })
}

pub fn is_sorted_u32(data: &[u32]) -> bool {
    data.windows(2).all(|window| window[0] <= window[1])
}
