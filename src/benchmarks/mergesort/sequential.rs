use crate::benchmarks::common::{
    checksum_sum_strings, checksum_sum_u32, emit_csv, generate_strings, generate_u32,
    is_sorted_strings, is_sorted_u32, maybe_load_data_strings, maybe_load_data_u32, Timer,
};

fn mergesort_inner<T: Ord + Clone>(arr: &mut [T], buf: &mut [T], cutoff: usize) {
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

    mergesort_inner(left, buf_left, cutoff);
    mergesort_inner(right, buf_right, cutoff);

    let (mut i, mut j, mut k) = (0, mid, 0);
    while i < mid && j < n {
        if arr[i] <= arr[j] {
            buf[k] = arr[i].clone();
            i += 1;
        } else {
            buf[k] = arr[j].clone();
            j += 1;
        }
        k += 1;
    }
    while i < mid {
        buf[k] = arr[i].clone();
        i += 1;
        k += 1;
    }
    while j < n {
        buf[k] = arr[j].clone();
        j += 1;
        k += 1;
    }

    arr.clone_from_slice(&buf[..n]);
}

pub fn mergesort_u32(arr: &mut [u32], cutoff: usize) {
    let n = arr.len();
    let mut buf = vec![0u32; n];
    mergesort_inner(arr, &mut buf, cutoff);
}

pub fn mergesort_strings(arr: &mut [String], cutoff: usize) {
    let n = arr.len();
    let mut buf = vec![String::new(); n];
    mergesort_inner(arr, &mut buf, cutoff);
}

pub fn run(input: &str, _threads: usize, run_id: usize) {
    let parts: Vec<&str> = input.split('_').collect();
    let n: usize = parts[0].parse().unwrap();
    let dtype = parts[1];
    let cutoff: usize = parts[2].strip_prefix("cutoff").unwrap().parse().unwrap();
    let seed = 42;

    match dtype {
        "u32" => {
            let mut data = maybe_load_data_u32().unwrap_or_else(|| generate_u32(n, seed));
            let expected_sum = checksum_sum_u32(&data);
            let timer = Timer::start();
            mergesort_u32(&mut data, cutoff);
            let elapsed = timer.elapsed_secs();
            assert!(is_sorted_u32(&data), "u32 mergesort not sorted");
            let actual_sum = checksum_sum_u32(&data);
            assert_eq!(actual_sum, expected_sum, "u32 checksum mismatch");
            emit_csv(
                "mergesort",
                input,
                "rust_seq",
                1,
                run_id,
                elapsed,
                actual_sum,
            );
        }
        "String" => {
            let mut data = maybe_load_data_strings().unwrap_or_else(|| generate_strings(n, seed));
            let expected_sum = checksum_sum_strings(&data);
            let timer = Timer::start();
            mergesort_strings(&mut data, cutoff);
            let elapsed = timer.elapsed_secs();
            assert!(is_sorted_strings(&data), "String mergesort not sorted");
            let actual_sum = checksum_sum_strings(&data);
            assert_eq!(actual_sum, expected_sum, "String checksum mismatch");
            emit_csv(
                "mergesort",
                input,
                "rust_seq",
                1,
                run_id,
                elapsed,
                actual_sum,
            );
        }
        _ => panic!("unknown dtype: {}", dtype),
    }
}
