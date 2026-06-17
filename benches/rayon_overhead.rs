use criterion::{criterion_group, criterion_main, BatchSize, Criterion};
use std::hint::black_box;
use std::time::Duration;
use rayon::prelude::*;
use benchrc::benchmarks::common::{histogram_uniform_1m, rayon_overhead_u32_10k};

fn rayon_join_empty(c: &mut Criterion) {
    c.bench_function("rayon_join_empty", |b| {
        b.iter(|| {
            rayon::join(|| {}, || {});
        });
    });
}

fn rayon_join_inc(c: &mut Criterion) {
    c.bench_function("rayon_join_increment", |b| {
        b.iter(|| {
            let mut a = 0;
            let mut b = 0;
            rayon::join(|| a = black_box(1), || b = black_box(2));
            black_box((a, b))
        });
    });
}

fn rayon_par_iter_small(c: &mut Criterion) {
    let data: Vec<u64> = (0..1000).collect();
    c.bench_function("rayon_par_iter_1k", |b| {
        b.iter(|| {
            let sum: u64 = data.par_iter().map(|x| black_box(x) * 2).sum();
            black_box(sum)
        });
    });
}

fn rayon_par_iter_medium(c: &mut Criterion) {
    let data: Vec<u64> = (0..100_000).collect();
    c.bench_function("rayon_par_iter_100k", |b| {
        b.iter(|| {
            let sum: u64 = data.par_iter().map(|x| black_box(x) * 2).sum();
            black_box(sum)
        });
    });
}

fn rayon_threadpool_spawn(c: &mut Criterion) {
    c.bench_function("rayon_threadpool_create", |b| {
        b.iter(|| {
            let pool = rayon::ThreadPoolBuilder::new()
                .num_threads(4)
                .build()
                .unwrap();
            pool.install(|| {});
            black_box(())
        });
    });
}

fn rayon_par_chunks_histogram(c: &mut Criterion) {
    let data = histogram_uniform_1m();
    c.bench_function("rayon_par_chunks_histogram", |b| {
        b.iter(|| {
            let result = data
                .par_chunks(250_000)
                .map(|chunk| {
                    let mut local = vec![0u64; 256];
                    for &value in chunk {
                        local[value as usize] += 1;
                    }
                    local
                })
                .reduce(
                    || vec![0u64; 256],
                    |mut left, right| {
                        for (left_value, &right_value) in left.iter_mut().zip(right.iter()) {
                            *left_value += right_value;
                        }
                        left
                    },
                );
            black_box(result);
        });
    });
}

fn rayon_join_recursive_mergesort(c: &mut Criterion) {
    fn mergesort(arr: &mut [u64], buf: &mut [u64], cutoff: usize) {
        let n = arr.len();
        if n <= 1 {
            return;
        }
        if n <= cutoff {
            arr.sort_unstable();
            return;
        }
        let mid = n / 2;
        let (left, right) = arr.split_at_mut(mid);
        let (buf_left, buf_right) = buf.split_at_mut(mid);
        rayon::join(
            || mergesort(left, buf_left, cutoff),
            || mergesort(right, buf_right, cutoff),
        );
        let (mut i, mut j, mut k) = (0, mid, 0);
        while i < mid && j < n {
            if arr[i] <= arr[j] {
                buf[k] = arr[i];
                i += 1;
            } else {
                buf[k] = arr[j];
                j += 1;
            }
            k += 1;
        }
        while i < mid {
            buf[k] = arr[i];
            i += 1;
            k += 1;
        }
        while j < n {
            buf[k] = arr[j];
            j += 1;
            k += 1;
        }
        arr.clone_from_slice(&buf[..n]);
    }

    let source: Vec<u64> = rayon_overhead_u32_10k()
        .iter()
        .copied()
        .map(|value| value as u64)
        .collect();

    c.bench_function("rayon_mergesort_10k_cutoff256", |b| {
        b.iter_batched(
            || (source.clone(), vec![0u64; source.len()]),
            |(mut values, mut buffer)| {
                mergesort(&mut values, &mut buffer, 256);
                black_box(values);
            },
            BatchSize::LargeInput,
        );
    });
}

criterion_group!(
    name = rayon_overhead;
    config = Criterion::default().sample_size(10).warm_up_time(Duration::from_secs(10)).measurement_time(Duration::from_secs(60));
    targets =
        rayon_join_empty,
        rayon_join_inc,
        rayon_par_iter_small,
        rayon_par_iter_medium,
        rayon_threadpool_spawn,
        rayon_par_chunks_histogram,
        rayon_join_recursive_mergesort,
);
criterion_main!(rayon_overhead);
