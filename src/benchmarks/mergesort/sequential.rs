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
    let mut buf = vec![0u32; arr.len()];
    mergesort_inner(arr, &mut buf, cutoff);
}
