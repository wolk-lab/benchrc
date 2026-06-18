use std::{
    env, fs,
    path::PathBuf,
    sync::OnceLock,
};

const FNV_OFFSET: u64 = 0xcbf29ce484222325;
const FNV_PRIME: u64 = 0x100000001b3;
pub const DATASETS_ENV: &str = "BENCHRC_DATASETS_DIR";

pub struct Graph {
    pub offsets: Vec<usize>,
    pub edges: Vec<u32>,
    pub num_nodes: usize,
}

fn datasets_dir() -> PathBuf {
    env::var_os(DATASETS_ENV)
        .map(PathBuf::from)
        .unwrap_or_else(|| PathBuf::from("datasets"))
}

fn dataset_path(name: &str) -> PathBuf {
    datasets_dir().join(name)
}

fn read_dataset(name: &str) -> Vec<u8> {
    let path = dataset_path(name);
    fs::read(&path).unwrap_or_else(|err| {
        panic!(
            "failed to read dataset {name} at {}: {err}. run `python3 scripts/generate_datasets.py` first",
            path.display()
        )
    })
}

fn load_u64_dataset(name: &str, expected_len: usize) -> Vec<u64> {
    let bytes = read_dataset(name);
    let expected_bytes = expected_len * std::mem::size_of::<u64>();
    assert_eq!(
        bytes.len(), expected_bytes,
        "dataset {name} has {} bytes, expected {expected_bytes}",
        bytes.len()
    );

    bytes
        .chunks_exact(8)
        .map(|chunk| u64::from_le_bytes(chunk.try_into().unwrap()))
        .collect()
}

fn load_u32_dataset(name: &str, expected_len: usize) -> Vec<u32> {
    let bytes = read_dataset(name);
    let expected_bytes = expected_len * std::mem::size_of::<u32>();
    assert_eq!(
        bytes.len(), expected_bytes,
        "dataset {name} has {} bytes, expected {expected_bytes}",
        bytes.len()
    );

    bytes
        .chunks_exact(4)
        .map(|chunk| u32::from_le_bytes(chunk.try_into().unwrap()))
        .collect()
}

fn load_f64_dataset(name: &str, expected_len: usize) -> Vec<f64> {
    let bytes = read_dataset(name);
    let expected_bytes = expected_len * std::mem::size_of::<f64>();
    assert_eq!(
        bytes.len(), expected_bytes,
        "dataset {name} has {} bytes, expected {expected_bytes}",
        bytes.len()
    );

    bytes
        .chunks_exact(8)
        .map(|chunk| f64::from_bits(u64::from_le_bytes(chunk.try_into().unwrap())))
        .collect()
}

fn load_graph_dataset(name: &str) -> Graph {
    let bytes = read_dataset(name);
    assert!(bytes.len() >= 24, "graph dataset {name} is too small");

    let num_nodes = u64::from_le_bytes(bytes[0..8].try_into().unwrap()) as usize;
    let offsets_len = u64::from_le_bytes(bytes[8..16].try_into().unwrap()) as usize;
    let edges_len = u64::from_le_bytes(bytes[16..24].try_into().unwrap()) as usize;

    let expected_bytes = 24 + offsets_len * 8 + edges_len * 4;
    assert_eq!(
        bytes.len(), expected_bytes,
        "graph dataset {name} has {} bytes, expected {expected_bytes}",
        bytes.len()
    );

    let offsets_start = 24;
    let edges_start = offsets_start + offsets_len * 8;

    let offsets = bytes[offsets_start..edges_start]
        .chunks_exact(8)
        .map(|chunk| u64::from_le_bytes(chunk.try_into().unwrap()) as usize)
        .collect();
    let edges = bytes[edges_start..]
        .chunks_exact(4)
        .map(|chunk| u32::from_le_bytes(chunk.try_into().unwrap()))
        .collect();

    Graph {
        offsets,
        edges,
        num_nodes,
    }
}

pub fn histogram_uniform_1m() -> &'static Vec<u64> {
    static DATA: OnceLock<Vec<u64>> = OnceLock::new();
    DATA.get_or_init(|| load_u64_dataset("histogram_uniform_1m.u64le.bin", 1_000_000))
}

pub fn histogram_uniform_1b() -> &'static Vec<u64> {
    static DATA: OnceLock<Vec<u64>> = OnceLock::new();
    DATA.get_or_init(|| load_u64_dataset("histogram_uniform_1b.u64le.bin", 1_000_000_000))
}

pub fn mergesort_u32_1b() -> &'static Vec<u32> {
    static DATA: OnceLock<Vec<u32>> = OnceLock::new();
    DATA.get_or_init(|| load_u32_dataset("mergesort_u32_1b.u32le.bin", 1_000_000_000))
}

pub fn stencil_f64_1b() -> &'static Vec<f64> {
    static DATA: OnceLock<Vec<f64>> = OnceLock::new();
    DATA.get_or_init(|| load_f64_dataset("stencil_f64_1b.f64le.bin", 1_000_000_000))
}

pub fn rayon_overhead_u32_10k() -> &'static Vec<u32> {
    static DATA: OnceLock<Vec<u32>> = OnceLock::new();
    DATA.get_or_init(|| load_u32_dataset("rayon_overhead_u32_10k.u32le.bin", 10_000))
}

pub fn bfs_graph_320m() -> &'static Graph {
    static DATA: OnceLock<Graph> = OnceLock::new();
    DATA.get_or_init(|| load_graph_dataset("bfs_320m_tree.graph.bin"))
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
