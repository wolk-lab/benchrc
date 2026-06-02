#pragma once

#include "common.hpp"

#include <algorithm>
#include <atomic>
#include <memory>
#include <mutex>
#include <vector>

#if __has_include(<omp.h>)
#include <omp.h>
#define SEMINAR_HAS_OPENMP 1
#else
#define SEMINAR_HAS_OPENMP 0
#endif

#if __has_include(<taskflow/taskflow.hpp>)
#include <taskflow/taskflow.hpp>
#define SEMINAR_HAS_TASKFLOW 1
#else
#define SEMINAR_HAS_TASKFLOW 0
#endif

namespace seminar {

inline Graph benchmark_graph(size_t num_nodes, size_t fanout) {
    std::vector<size_t> offsets;
    std::vector<uint32_t> edges;
    offsets.reserve(num_nodes + 1);
    offsets.push_back(0);

    for (size_t node = 0; node < num_nodes; node++) {
        for (size_t step = 1; step <= fanout; step++) {
            size_t neighbor = node + step;
            if (neighbor < num_nodes) {
                edges.push_back(static_cast<uint32_t>(neighbor));
            }
        }
        offsets.push_back(edges.size());
    }

    return Graph{std::move(offsets), std::move(edges), num_nodes};
}

#if SEMINAR_HAS_OPENMP
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

inline uint64_t bfs_openmp(const Graph& graph, uint32_t source, int threads) {
    omp_set_num_threads(threads);

    size_t n = graph.num_nodes;
    auto visited = std::make_unique<std::atomic<uint32_t>[]>(n);
    for (size_t i = 0; i < n; i++) {
        visited[i].store(0, std::memory_order_relaxed);
    }

    std::vector<uint32_t> frontier;
    std::vector<uint32_t> next_frontier;
    uint64_t visited_count = 0;

    visited[source].store(1, std::memory_order_relaxed);
    frontier.push_back(source);
    visited_count++;

    while (!frontier.empty()) {
        next_frontier.clear();

        #pragma omp parallel
        {
            std::vector<uint32_t> local_next;

            #pragma omp for nowait
            for (size_t idx = 0; idx < frontier.size(); idx++) {
                uint32_t node = frontier[idx];
                size_t start = graph.offsets[node];
                size_t end = graph.offsets[node + 1];
                for (size_t edge = start; edge < end; edge++) {
                    uint32_t neighbor = graph.edges[edge];
                    uint32_t expected = 0;
                    if (visited[neighbor].compare_exchange_strong(expected, 1,
                                                                  std::memory_order_acq_rel)) {
                        local_next.push_back(neighbor);
                    }
                }
            }

            #pragma omp critical
            next_frontier.insert(next_frontier.end(), local_next.begin(), local_next.end());
        }

        frontier.swap(next_frontier);
        visited_count += frontier.size();
    }

    return visited_count;
}
#endif

#if SEMINAR_HAS_TASKFLOW
inline std::vector<uint64_t> histogram_taskflow(const std::vector<uint64_t>& data, uint64_t buckets,
                                                unsigned num_threads, unsigned num_tasks) {
    size_t n = data.size();
    std::vector<std::vector<uint64_t>> locals(num_tasks, std::vector<uint64_t>(buckets, 0));

    tf::Executor executor(num_threads);
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
inline void mergesort_taskflow(std::vector<T>& data, size_t cutoff, unsigned num_threads) {
    tf::Executor executor(num_threads);
    tf::Taskflow taskflow;
    std::vector<T> buf(data.size());
    build_mergesort_taskflow(taskflow, data.data(), buf.data(), data.size(), cutoff);
    executor.run(taskflow).wait();
}

inline std::vector<double> stencil_taskflow(const std::vector<double>& values, size_t iterations,
                                            size_t radius, unsigned num_threads) {
    size_t n = values.size();
    std::vector<double> current = values;
    std::vector<double> next(n, 0.0);

    tf::Executor executor(num_threads);
    tf::Taskflow taskflow;
    std::vector<tf::Task> stages;

    for (size_t iter = 0; iter < iterations; iter++) {
        auto stage = taskflow.emplace([&current, &next, n, radius, num_threads]() {
            #pragma omp parallel for num_threads(num_threads)
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
        });
        stages.push_back(stage);
    }

    for (size_t i = 1; i < stages.size(); i++) {
        stages[i - 1].precede(stages[i]);
    }

    executor.run(taskflow).wait();
    return current;
}

inline uint64_t bfs_taskflow(const Graph& graph, uint32_t source, unsigned num_threads) {
    size_t n = graph.num_nodes;
    auto visited = std::make_unique<std::atomic<uint32_t>[]>(n);
    for (size_t i = 0; i < n; i++) {
        visited[i].store(0, std::memory_order_relaxed);
    }

    std::vector<uint32_t> frontier;
    uint64_t visited_count = 0;

    visited[source].store(1, std::memory_order_relaxed);
    frontier.push_back(source);
    visited_count++;

    tf::Executor executor(num_threads);

    while (!frontier.empty()) {
        std::vector<uint32_t> next_frontier;
        std::mutex mutex;

        size_t num_tasks = std::min(frontier.size(), static_cast<size_t>(num_threads * 4));
        size_t chunk_size = (frontier.size() + num_tasks - 1) / num_tasks;

        tf::Taskflow taskflow;
        for (size_t task = 0; task < num_tasks; task++) {
            size_t start = task * chunk_size;
            size_t end = std::min(start + chunk_size, frontier.size());

            taskflow.emplace([&frontier, &next_frontier, &graph, &visited, &mutex, start, end]() {
                std::vector<uint32_t> local_next;
                for (size_t idx = start; idx < end; idx++) {
                    uint32_t node = frontier[idx];
                    size_t edge_start = graph.offsets[node];
                    size_t edge_end = graph.offsets[node + 1];
                    for (size_t edge = edge_start; edge < edge_end; edge++) {
                        uint32_t neighbor = graph.edges[edge];
                        uint32_t expected = 0;
                        if (visited[neighbor].compare_exchange_strong(expected, 1,
                                                                      std::memory_order_acq_rel)) {
                            local_next.push_back(neighbor);
                        }
                    }
                }
                if (!local_next.empty()) {
                    std::lock_guard<std::mutex> lock(mutex);
                    next_frontier.insert(next_frontier.end(), local_next.begin(), local_next.end());
                }
            });
        }

        executor.run(taskflow).wait();
        frontier.swap(next_frontier);
        visited_count += frontier.size();
    }

    return visited_count;
}
#endif

}  // namespace seminar
