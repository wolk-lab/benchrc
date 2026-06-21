use rayon::prelude::*;

pub fn histogram(data: &[u64], buckets: usize) -> Vec<u64> {
    let num_threads = rayon::current_num_threads();

    let chunk_size = data.len().div_ceil(num_threads);

    data.par_chunks(chunk_size)
        .map(|chunk| {
            let mut local = vec![0u64; buckets];

            for &value in chunk {
                local[value as usize % buckets] += 1;
            }

            local
        })
        .reduce(
            || vec![0u64; buckets],
            |mut left, right| {
                for (left_value, &right_value) in left.iter_mut().zip(right.iter()) {
                    *left_value += right_value;
                }

                left
            },
        )
}
