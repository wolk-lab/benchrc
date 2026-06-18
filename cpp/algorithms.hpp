#pragma once

#include "common.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <omp.h>
#include <taskflow/taskflow.hpp>
#include <vector>

namespace seminar {

inline std::vector<uint64_t> histogram_sequential(const std::vector<uint64_t>& data,
                                                  uint64_t buckets) {
    std::vector<uint64_t> hist(buckets, 0);
    for (uint64_t value : data) {
        hist[static_cast<size_t>(value % buckets)]++;
    }
    return hist;
}

inline std::vector<uint64_t> histogram_openmp(const std::vector<uint64_t>& data, uint64_t buckets,
                                               int threads) {
    omp_set_num_threads(threads);

    size_t n = data.size();
    std::vector<uint64_t> global(buckets, 0);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int nt = omp_get_num_threads();
        std::vector<uint64_t> local(buckets, 0);

        size_t chunk_size = (n + static_cast<size_t>(nt) - 1) / static_cast<size_t>(nt);
        size_t start = static_cast<size_t>(tid) * chunk_size;
        size_t end = std::min(start + chunk_size, n);

        for (size_t i = start; i < end; i++) {
            local[data[i] % buckets]++;
        }

        #pragma omp critical
        for (size_t bucket = 0; bucket < buckets; bucket++) {
            global[bucket] += local[bucket];
        }
    }

    return global;
}

template <typename T>
inline void mergesort_sequential_inner(T* arr, T* buf, size_t n, size_t cutoff) {
    if (n <= 1) {
        return;
    }
    if (n <= cutoff) {
        std::sort(arr, arr + n);
        return;
    }

    size_t mid = n / 2;
    mergesort_sequential_inner(arr, buf, mid, cutoff);
    mergesort_sequential_inner(arr + mid, buf + mid, n - mid, cutoff);

    size_t i = 0;
    size_t j = mid;
    size_t k = 0;
    while (i < mid && j < n) {
        if (arr[i] <= arr[j]) {
            buf[k++] = arr[i++];
        } else {
            buf[k++] = arr[j++];
        }
    }
    while (i < mid) {
        buf[k++] = arr[i++];
    }
    while (j < n) {
        buf[k++] = arr[j++];
    }

    std::copy(buf, buf + n, arr);
}

template <typename T>
inline void mergesort_sequential(std::vector<T>& data, size_t cutoff) {
    std::vector<T> buf(data.size());
    mergesort_sequential_inner(data.data(), buf.data(), data.size(), cutoff);
}

template <typename T>
inline void mergesort_openmp_inner(T* arr, T* buf, size_t n, size_t cutoff) {
    if (n <= 1) {
        return;
    }
    if (n <= cutoff) {
        std::sort(arr, arr + n);
        return;
    }

    size_t mid = n / 2;

    #pragma omp task
    mergesort_openmp_inner(arr, buf, mid, cutoff);

    #pragma omp task
    mergesort_openmp_inner(arr + mid, buf + mid, n - mid, cutoff);

    #pragma omp taskwait

    size_t i = 0;
    size_t j = mid;
    size_t k = 0;
    while (i < mid && j < n) {
        if (arr[i] <= arr[j]) {
            buf[k++] = arr[i++];
        } else {
            buf[k++] = arr[j++];
        }
    }
    while (i < mid) {
        buf[k++] = arr[i++];
    }
    while (j < n) {
        buf[k++] = arr[j++];
    }

    std::copy(buf, buf + n, arr);
}

template <typename T>
inline void mergesort_openmp(std::vector<T>& data, size_t cutoff, int threads) {
    omp_set_num_threads(threads);
    std::vector<T> buf(data.size());
    #pragma omp parallel
    {
        #pragma omp single
        mergesort_openmp_inner(data.data(), buf.data(), data.size(), cutoff);
    }
}

inline std::vector<double> stencil_sequential(const std::vector<double>& values,
                                              size_t iterations, size_t radius) {
    size_t n = values.size();
    std::vector<double> current = values;
    std::vector<double> next(n, 0.0);

    for (size_t iter = 0; iter < iterations; iter++) {
        for (size_t i = 0; i < n; i++) {
            size_t start = (i < radius) ? 0 : (i - radius);
            size_t end = std::min(i + radius + 1, n);
            size_t count = end - start;
            double sum = 0.0;
            for (size_t j = start; j < end; j++) {
                sum += current[j];
            }
            next[i] = sum / static_cast<double>(count);
        }
        std::swap(current, next);
    }

    return current;
}

inline std::vector<double> stencil_openmp(const std::vector<double>& values, size_t iterations,
                                           size_t radius, int threads) {
    omp_set_num_threads(threads);

    size_t n = values.size();
    std::vector<double> current = values;
    std::vector<double> next(n, 0.0);

    for (size_t iter = 0; iter < iterations; iter++) {
        #pragma omp parallel for
        for (size_t i = 0; i < n; i++) {
            size_t start = (i < radius) ? 0 : (i - radius);
            size_t end = std::min(i + radius + 1, n);
            size_t count = end - start;
            double sum = 0.0;
            for (size_t j = start; j < end; j++) {
                sum += current[j];
            }
            next[i] = sum / static_cast<double>(count);
        }
        std::swap(current, next);
    }

    return current;
}

inline uint64_t bfs_sequential(const Graph& graph, uint32_t source) {
    std::vector<uint8_t> visited(graph.num_nodes, 0);
    std::vector<uint32_t> frontier{source};
    uint64_t visited_count = 1;

    visited[source] = 1;

    while (!frontier.empty()) {
        std::vector<uint32_t> next_frontier;
        for (uint32_t node : frontier) {
            size_t start = graph.offsets[node];
            size_t end = graph.offsets[node + 1];
            for (size_t edge = start; edge < end; edge++) {
                uint32_t neighbor = graph.edges[edge];
                if (!visited[neighbor]) {
                    visited[neighbor] = 1;
                    next_frontier.push_back(neighbor);
                    visited_count++;
                }
            }
        }
        frontier.swap(next_frontier);
    }

    return visited_count;
}

inline uint64_t bfs_openmp(const Graph& graph, uint32_t source, int threads) {
    omp_set_num_threads(threads);

    size_t n = graph.num_nodes;
    auto visited = std::make_unique<std::atomic<bool>[]>(n);
    for (size_t i = 0; i < n; i++) {
        visited[i].store(false, std::memory_order_relaxed);
    }

    std::vector<uint32_t> frontier;
    uint64_t visited_count = 1;

    visited[source].store(true, std::memory_order_relaxed);
    frontier.push_back(source);

    while (!frontier.empty()) {
        size_t fsize = frontier.size();
        size_t nthreads = static_cast<size_t>(threads);

        std::vector<std::vector<uint32_t>> thread_locals(nthreads);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            auto& local = thread_locals[static_cast<size_t>(tid)];
            local.clear();
            local.reserve(fsize * 2 / nthreads + 64);

            #pragma omp for schedule(static)
            for (size_t idx = 0; idx < fsize; idx++) {
                uint32_t node = frontier[idx];
                size_t start = graph.offsets[node];
                size_t end = graph.offsets[node + 1];
                for (size_t edge = start; edge < end; edge++) {
                    uint32_t neighbor = graph.edges[edge];
                    if (!visited[neighbor].exchange(true, std::memory_order_acq_rel)) {
                        local.push_back(neighbor);
                    }
                }
            }
        }

        std::vector<size_t> offsets(nthreads + 1);
        offsets[0] = 0;
        for (size_t i = 0; i < nthreads; i++) {
            offsets[i + 1] = offsets[i] + thread_locals[i].size();
        }
        size_t total = offsets[nthreads];

        std::vector<uint32_t> next(total);

        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < nthreads; i++) {
            std::copy(thread_locals[i].begin(), thread_locals[i].end(),
                      next.begin() + offsets[i]);
        }

        visited_count += next.size();
        frontier.swap(next);
    }

    return visited_count;
}

inline std::vector<uint64_t> histogram_taskflow(const std::vector<uint64_t>& data, uint64_t buckets,
                                                tf::Executor& executor, unsigned num_tasks) {
    size_t n = data.size();
    std::vector<std::vector<uint64_t>> locals(num_tasks, std::vector<uint64_t>(buckets, 0));

    tf::Taskflow taskflow;
    std::vector<tf::Task> tasks;
    size_t chunk_size = (n + num_tasks - 1) / num_tasks;

    for (unsigned task = 0; task < num_tasks; task++) {
        size_t start = static_cast<size_t>(task) * chunk_size;
        size_t end = std::min(start + chunk_size, n);
        tasks.push_back(taskflow.emplace([&data, &locals, task, start, end, buckets]() {
            for (size_t i = start; i < end; i++) {
                locals[task][data[i] % buckets]++;
            }
        }));
    }

    std::vector<uint64_t> global(buckets, 0);
    auto reduce_task = taskflow.emplace([&locals, &global, num_tasks, buckets]() {
        for (unsigned task = 0; task < num_tasks; task++) {
            for (size_t bucket = 0; bucket < buckets; bucket++) {
                global[bucket] += locals[task][bucket];
            }
        }
    });

    for (auto& task : tasks) {
        task.precede(reduce_task);
    }

    executor.run(taskflow).wait();
    return global;
}

template <typename T>
inline tf::Task build_mergesort_taskflow(tf::Taskflow& taskflow, T* arr, T* buf,
                                         size_t n, size_t cutoff) {
    if (n <= 1) {
        return taskflow.emplace([]() {});
    }
    if (n <= cutoff) {
        return taskflow.emplace([arr, n]() { std::sort(arr, arr + n); });
    }

    size_t mid = n / 2;
    auto left = build_mergesort_taskflow(taskflow, arr, buf, mid, cutoff);
    auto right = build_mergesort_taskflow(taskflow, arr + mid, buf + mid, n - mid, cutoff);

    auto merge = taskflow.emplace([arr, buf, mid, n]() {
        size_t i = 0;
        size_t j = mid;
        size_t k = 0;
        while (i < mid && j < n) {
            if (arr[i] <= arr[j]) {
                buf[k++] = arr[i++];
            } else {
                buf[k++] = arr[j++];
            }
        }
        while (i < mid) {
            buf[k++] = arr[i++];
        }
        while (j < n) {
            buf[k++] = arr[j++];
        }
        std::copy(buf, buf + n, arr);
    });

    left.precede(merge);
    right.precede(merge);
    return merge;
}

template <typename T>
inline void mergesort_taskflow(std::vector<T>& data, size_t cutoff, tf::Executor& executor) {
    tf::Taskflow taskflow;
    std::vector<T> buf(data.size());
    build_mergesort_taskflow(taskflow, data.data(), buf.data(), data.size(), cutoff);
    executor.run(taskflow).wait();
}

inline std::vector<double> stencil_taskflow(const std::vector<double>& values, size_t iterations,
                                            size_t radius, tf::Executor& executor,
                                            unsigned num_threads) {
    size_t n = values.size();
    std::vector<double> current = values;
    std::vector<double> next(n, 0.0);

    tf::Taskflow taskflow;
    size_t num_tasks = std::max<size_t>(1, std::min(n, static_cast<size_t>(num_threads)));
    size_t chunk_size = (n + num_tasks - 1) / num_tasks;
    tf::Task previous_swap;
    bool has_previous_swap = false;

    for (size_t iter = 0; iter < iterations; iter++) {
        std::vector<tf::Task> stage_tasks;
        stage_tasks.reserve(num_tasks);
        for (size_t task = 0; task < num_tasks; task++) {
            size_t start = task * chunk_size;
            size_t end = std::min(start + chunk_size, n);
            auto stage_task = taskflow.emplace([&current, &next, start, end, radius, n]() {
                for (size_t i = start; i < end; i++) {
                    size_t window_start = (i < radius) ? 0 : (i - radius);
                    size_t window_end = std::min(i + radius + 1, n);
                    size_t count = window_end - window_start;
                    double sum = 0.0;
                    for (size_t j = window_start; j < window_end; j++) {
                        sum += current[j];
                    }
                    next[i] = sum / static_cast<double>(count);
                }
            });
            if (has_previous_swap) {
                previous_swap.precede(stage_task);
            }
            stage_tasks.push_back(stage_task);
        }

        auto swap_task = taskflow.emplace([&current, &next]() { std::swap(current, next); });
        for (auto& stage_task : stage_tasks) {
            stage_task.precede(swap_task);
        }
        previous_swap = swap_task;
        has_previous_swap = true;
    }

    executor.run(taskflow).wait();
    return current;
}

inline uint64_t bfs_taskflow(const Graph& graph, uint32_t source, tf::Executor& executor,
                             unsigned num_threads) {
    size_t n = graph.num_nodes;
    auto visited = std::make_unique<std::atomic<bool>[]>(n);
    for (size_t i = 0; i < n; i++) {
        visited[i].store(false, std::memory_order_relaxed);
    }

    std::vector<uint32_t> frontier;
    uint64_t visited_count = 1;

    visited[source].store(true, std::memory_order_relaxed);
    frontier.push_back(source);

    while (!frontier.empty()) {
        size_t fsize = frontier.size();
        unsigned num_tasks = static_cast<unsigned>(std::max<size_t>(1, std::min(fsize, static_cast<size_t>(num_threads * 4))));
        size_t chunk_size = (fsize + num_tasks - 1) / num_tasks;

        std::vector<std::vector<uint32_t>> task_locals(num_tasks);

        tf::Taskflow taskflow;

        std::vector<tf::Task> collect_tasks;
        for (unsigned task = 0; task < num_tasks; task++) {
            size_t start = task * chunk_size;
            size_t end = std::min(start + chunk_size, fsize);

            collect_tasks.push_back(taskflow.emplace([&frontier, &graph, &visited, &task_locals, task, start, end]() {
                auto& local = task_locals[task];
                local.clear();
                for (size_t idx = start; idx < end; idx++) {
                    uint32_t node = frontier[idx];
                    size_t edge_start = graph.offsets[node];
                    size_t edge_end = graph.offsets[node + 1];
                    for (size_t edge = edge_start; edge < edge_end; edge++) {
                        uint32_t neighbor = graph.edges[edge];
                        if (!visited[neighbor].exchange(true, std::memory_order_acq_rel)) {
                            local.push_back(neighbor);
                        }
                    }
                }
            }));
        }

        std::vector<size_t> offsets(num_tasks + 1);
        std::vector<uint32_t> next;
        auto merge_prep = taskflow.emplace([&task_locals, &offsets, &next, num_tasks]() {
            offsets[0] = 0;
            for (unsigned i = 0; i < num_tasks; i++) {
                offsets[i + 1] = offsets[i] + task_locals[i].size();
            }
            next.resize(offsets[num_tasks]);
        });

        std::vector<tf::Task> copy_tasks;
        for (unsigned task = 0; task < num_tasks; task++) {
            copy_tasks.push_back(taskflow.emplace([&task_locals, &next, &offsets, task]() {
                std::copy(task_locals[task].begin(), task_locals[task].end(),
                          next.begin() + offsets[task]);
            }));
        }

        for (auto& t : collect_tasks) t.precede(merge_prep);
        for (auto& t : copy_tasks) merge_prep.precede(t);

        executor.run(taskflow).wait();

        visited_count += next.size();
        frontier.swap(next);
    }

    return visited_count;
}

}  // namespace seminar
