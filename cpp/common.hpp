#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <random>
#include <vector>

static constexpr uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
static constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;

static std::vector<uint64_t> generate_uniform(size_t n, uint64_t buckets, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<uint64_t> dist(0, buckets - 1);
    std::vector<uint64_t> data(n);
    for (auto& value : data) value = dist(rng);
    return data;
}

static std::vector<uint64_t> generate_zipf(size_t n, uint64_t buckets, double alpha, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> uniform(0.0, 1.0);

    std::vector<double> cdf(buckets);
    double denom = 0.0;
    for (uint64_t i = 1; i <= buckets; i++) {
        denom += 1.0 / std::pow(i, alpha);
    }

    double cum = 0.0;
    for (uint64_t i = 0; i < buckets; i++) {
        cum += 1.0 / std::pow(i + 1, alpha) / denom;
        cdf[i] = cum;
    }

    std::vector<uint64_t> data(n);
    for (auto& value : data) {
        double sample = uniform(rng);
        uint64_t lo = 0;
        uint64_t hi = buckets - 1;
        while (lo < hi) {
            uint64_t mid = lo + (hi - lo) / 2;
            if (cdf[mid] <= sample) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }
        value = lo;
    }

    return data;
}

static std::vector<uint32_t> generate_u32(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::vector<uint32_t> data(n);
    for (auto& value : data) value = static_cast<uint32_t>(rng());
    return data;
}

static std::vector<double> generate_f64(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    std::vector<double> data(n);
    for (auto& value : data) value = dist(rng);
    return data;
}

static uint64_t mix64(uint64_t value) {
    value += 0x9e3779b97f4a7c15ULL;
    value = (value ^ (value >> 30)) * 0xbf58476d1ce4e5b9ULL;
    value = (value ^ (value >> 27)) * 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}

static uint64_t checksum_histogram(const std::vector<uint64_t>& hist) {
    uint64_t sum = 0;
    for (auto value : hist) sum += value;
    return sum;
}

static uint64_t checksum_sum_u32(const std::vector<uint32_t>& data) {
    uint64_t sum = 0;
    for (auto value : data) sum += mix64(static_cast<uint64_t>(value));
    return sum;
}

static uint64_t checksum_sum_f64(const std::vector<double>& data) {
    uint64_t hash = FNV_OFFSET;
    for (size_t index = 0; index < data.size(); index++) {
        hash ^= mix64(std::bit_cast<uint64_t>(data[index]) ^ static_cast<uint64_t>(index));
        hash *= FNV_PRIME;
    }
    return hash;
}

template <typename T>
static bool is_sorted(const std::vector<T>& data) {
    for (size_t i = 1; i < data.size(); i++) {
        if (data[i - 1] > data[i]) return false;
    }
    return true;
}

struct Graph {
    std::vector<size_t> offsets;
    std::vector<uint32_t> edges;
    size_t num_nodes;
};
