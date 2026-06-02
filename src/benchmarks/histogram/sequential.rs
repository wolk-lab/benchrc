pub fn histogram(data: &[u64], buckets: usize) -> Vec<u64> {
    let mut hist = vec![0u64; buckets];
    for &value in data {
        hist[value as usize % buckets] += 1;
    }
    hist
}
