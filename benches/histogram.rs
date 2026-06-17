mod common;

use common::{RAYON_THREAD_COUNTS, configure_group, default_criterion, set_element_throughput};
use criterion::{BenchmarkId, criterion_group, criterion_main};
use benchrc::benchmarks::{
    common::{checksum_histogram, histogram_uniform_1b, histogram_uniform_1m},
    histogram::{rayon as histogram_rayon, sequential as histogram_seq},
};
use std::hint::black_box;

fn histogram_benches(c: &mut criterion::Criterion) {
    // Verify correctness on 1M before benchmarking
    let verify_data = histogram_uniform_1m();
    let seq_result = histogram_seq::histogram(verify_data, 256);
    assert_eq!(checksum_histogram(&seq_result), verify_data.len() as u64, "sequential histogram verification failed");
    let rayon_result = histogram_rayon::histogram(verify_data, 256);
    assert_eq!(checksum_histogram(&rayon_result), verify_data.len() as u64, "rayon histogram verification failed");

    let cases = [
        ("1M_uniform_256", histogram_uniform_1m().as_slice()),
        ("1B_uniform_256", histogram_uniform_1b().as_slice()),
    ];

    let mut group = c.benchmark_group("histogram");
    configure_group(&mut group);

    for (label, data) in &cases {
        set_element_throughput(&mut group, data.len());

        group.bench_function(BenchmarkId::new("seq", label), |b| {
            b.iter(|| black_box(histogram_seq::histogram(black_box(data), 256)));
        });

        for &threads in &RAYON_THREAD_COUNTS {
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(threads)
                .build()
                .unwrap();
            group.bench_function(BenchmarkId::new(format!("rayon_{threads}t"), label), |b| {
                b.iter(|| {
                    pool.install(|| black_box(histogram_rayon::histogram(black_box(data), 256)));
                });
            });
        }
    }

    group.finish();
}

criterion_group!(name = histogram; config = default_criterion(); targets = histogram_benches);
criterion_main!(histogram);
