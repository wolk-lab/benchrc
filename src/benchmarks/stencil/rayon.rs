use rayon::prelude::*;

pub fn stencil(values: &[f64], iterations: usize, radius: usize) -> Vec<f64> {
    let n = values.len();
    let mut current = values.to_vec();
    let mut next = vec![0.0; n];

    for _ in 0..iterations {
        next.par_iter_mut().enumerate().for_each(|(index, item)| {
            let start = index.saturating_sub(radius);
            let end = (index + radius + 1).min(n);
            let count = end - start;
            let sum: f64 = current[start..end].iter().sum();
            *item = sum / count as f64;
        });
        std::mem::swap(&mut current, &mut next);
    }

    current
}
