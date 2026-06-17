use criterion::{Criterion, Throughput};
use std::time::Duration;

pub const RAYON_THREAD_COUNTS: [usize; 6] = [1, 2, 4, 8, 16, 32];

pub fn configure_group(group: &mut criterion::BenchmarkGroup<'_, criterion::measurement::WallTime>) {
    group.sample_size(5);
    group.warm_up_time(Duration::from_secs(10));
    group.measurement_time(Duration::from_secs(60));
}

pub fn set_element_throughput(
    group: &mut criterion::BenchmarkGroup<'_, criterion::measurement::WallTime>,
    elements: usize,
) {
    group.throughput(Throughput::Elements(elements as u64));
}

pub fn default_criterion() -> Criterion {
    Criterion::default()
}
