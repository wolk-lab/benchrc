use crate::benchmarks::common::{
    checksum_histogram, emit_csv, generate_uniform, generate_zipf, maybe_load_data_u64, Timer,
};
use rayon::prelude::*;

pub fn histogram(data: &[u64], buckets: usize) -> Vec<u64> {
    let num_threads = rayon::current_num_threads();
    let chunk_size = data.len().div_ceil(num_threads);

    data.par_chunks(chunk_size)
        .map(|chunk| {
            let mut local = vec![0u64; buckets];
            for &v in chunk {
                local[v as usize % buckets] += 1;
            }
            local
        })
        .reduce(
            || vec![0u64; buckets],
            |mut a, b| {
                for (ai, &bi) in a.iter_mut().zip(b.iter()) {
                    *ai += bi;
                }
                a
            },
        )
}

pub fn run(input: &str, threads: usize, run_id: usize) {
    let pool = rayon::ThreadPoolBuilder::new()
        .num_threads(threads)
        .build()
        .unwrap();

    pool.install(|| {
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

        let chk = checksum_histogram(&result);
        assert_eq!(chk, n as u64, "histogram checksum failed");

        emit_csv(
            "histogram",
            input,
            "rust_rayon",
            threads,
            run_id,
            elapsed,
            chk,
        );
    });
}
