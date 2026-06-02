use rand::RngExt;
use rand::SeedableRng;
use rand_distr::{Distribution, Zipf};
use std::fs;
use std::time::Instant;

const FNV_OFFSET: u64 = 0xcbf29ce484222325;
const FNV_PRIME: u64 = 0x100000001b3;

/// CSR-format graph for BFS.
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

pub fn generate_strings(n: usize, seed: u64) -> Vec<String> {
    let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
    let chars: Vec<char> = "abcdefghijklmnopqrstuvwxyz0123456789".chars().collect();
    (0..n)
        .map(|_| {
            let len = rng.random_range(8..=64);
            (0..len)
                .map(|_| chars[rng.random_range(0..chars.len())])
                .collect()
        })
        .collect()
}

fn read_u64(bytes: &[u8], offset: &mut usize) -> u64 {
    let end = *offset + 8;
    assert!(end <= bytes.len(), "unexpected end of data file");
    let value = u64::from_le_bytes(bytes[*offset..end].try_into().unwrap());
    *offset = end;
    value
}

pub fn load_data_u64(path: &str) -> Vec<u64> {
    let bytes = fs::read(path).expect("failed to read data file");
    assert_eq!(bytes.len() % 8, 0, "data file size not aligned to u64");
    let n = bytes.len() / 8;
    let mut data = Vec::with_capacity(n);
    for chunk in bytes.chunks_exact(8) {
        data.push(u64::from_le_bytes(chunk.try_into().unwrap()));
    }
    data
}

pub fn load_data_u32(path: &str) -> Vec<u32> {
    let bytes = fs::read(path).expect("failed to read data file");
    assert_eq!(bytes.len() % 4, 0, "data file size not aligned to u32");
    let n = bytes.len() / 4;
    let mut data = Vec::with_capacity(n);
    for chunk in bytes.chunks_exact(4) {
        data.push(u32::from_le_bytes(chunk.try_into().unwrap()));
    }
    data
}

pub fn load_data_f64(path: &str) -> Vec<f64> {
    let bytes = fs::read(path).expect("failed to read data file");
    assert_eq!(bytes.len() % 8, 0, "data file size not aligned to f64");
    let n = bytes.len() / 8;
    let mut data = Vec::with_capacity(n);
    for chunk in bytes.chunks_exact(8) {
        data.push(f64::from_le_bytes(chunk.try_into().unwrap()));
    }
    data
}

pub fn load_data_strings(path: &str) -> Vec<String> {
    let bytes = fs::read(path).expect("failed to read data file");
    let mut offset = 0;
    let n = read_u64(&bytes, &mut offset) as usize;
    let mut data = Vec::with_capacity(n);

    for _ in 0..n {
        let len = read_u64(&bytes, &mut offset) as usize;
        let end = offset + len;
        assert!(end <= bytes.len(), "string data file truncated");
        let value = String::from_utf8(bytes[offset..end].to_vec())
            .expect("invalid utf-8 in string data file");
        data.push(value);
        offset = end;
    }

    assert_eq!(offset, bytes.len(), "string data file has trailing bytes");
    data
}

pub fn maybe_load_data_u64() -> Option<Vec<u64>> {
    std::env::var("DATA_FILE").ok().map(|p| load_data_u64(&p))
}

pub fn maybe_load_data_u32() -> Option<Vec<u32>> {
    std::env::var("DATA_FILE").ok().map(|p| load_data_u32(&p))
}

pub fn maybe_load_data_f64() -> Option<Vec<f64>> {
    std::env::var("DATA_FILE").ok().map(|p| load_data_f64(&p))
}

pub fn maybe_load_data_strings() -> Option<Vec<String>> {
    std::env::var("DATA_FILE")
        .ok()
        .map(|p| load_data_strings(&p))
}

pub fn generate_f64(n: usize, seed: u64) -> Vec<f64> {
    let mut rng = rand::rngs::StdRng::seed_from_u64(seed);
    (0..n).map(|_| rng.random()).collect()
}

pub struct Timer(Instant);

impl Timer {
    pub fn start() -> Self {
        Self(Instant::now())
    }

    pub fn elapsed_secs(&self) -> f64 {
        self.0.elapsed().as_secs_f64()
    }
}

pub fn emit_csv(
    benchmark: &str,
    input: &str,
    implementation: &str,
    threads: usize,
    run: usize,
    time_secs: f64,
    checksum: u64,
) {
    println!(
        "{},{},{},{},{},{:.6},{}",
        benchmark, input, implementation, threads, run, time_secs, checksum
    );
}

fn mix64(mut value: u64) -> u64 {
    value = value.wrapping_add(0x9e3779b97f4a7c15);
    value = (value ^ (value >> 30)).wrapping_mul(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)).wrapping_mul(0x94d049bb133111eb);
    value ^ (value >> 31)
}

fn hash_bytes(bytes: &[u8]) -> u64 {
    let mut hash = FNV_OFFSET;
    for &byte in bytes {
        hash ^= byte as u64;
        hash = hash.wrapping_mul(FNV_PRIME);
    }
    mix64(hash ^ bytes.len() as u64)
}

pub fn checksum_histogram(hist: &[u64]) -> u64 {
    hist.iter().sum()
}

pub fn checksum_sum_u32(data: &[u32]) -> u64 {
    data.iter()
        .fold(0u64, |sum, &value| sum.wrapping_add(mix64(value as u64)))
}

pub fn checksum_sum_strings(data: &[String]) -> u64 {
    data.iter().fold(0u64, |sum, value| {
        sum.wrapping_add(hash_bytes(value.as_bytes()))
    })
}

pub fn checksum_sum_f64(data: &[f64]) -> u64 {
    data.iter()
        .enumerate()
        .fold(FNV_OFFSET, |hash, (index, &value)| {
            (hash ^ mix64(value.to_bits() ^ index as u64)).wrapping_mul(FNV_PRIME)
        })
}

pub fn is_sorted_u32(data: &[u32]) -> bool {
    data.windows(2).all(|w| w[0] <= w[1])
}

pub fn is_sorted_strings(data: &[String]) -> bool {
    data.windows(2).all(|w| w[0] <= w[1])
}

pub fn load_graph(path: &str) -> Graph {
    let content = fs::read_to_string(path).expect("failed to read graph file");
    let mut lines = content.lines();

    let header: Vec<usize> = lines
        .next()
        .expect("empty graph file")
        .split_whitespace()
        .map(|s| s.parse().unwrap())
        .collect();
    let num_nodes = header[0];
    let num_edges = header[1];

    let mut offsets = vec![0usize; num_nodes + 1];
    for (i, line) in lines.by_ref().take(num_nodes).enumerate() {
        offsets[i] = line.trim().parse().unwrap();
    }
    offsets[num_nodes] = num_edges;

    let edges: Vec<u32> = lines.map(|line| line.trim().parse().unwrap()).collect();

    assert_eq!(edges.len(), num_edges, "edge count mismatch");

    Graph {
        offsets,
        edges,
        num_nodes,
    }
}
