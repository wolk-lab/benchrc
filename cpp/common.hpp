#pragma once

#include <algorithm>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <vector>

static constexpr uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
static constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;

struct Timer {
    std::chrono::high_resolution_clock::time_point start;

    Timer() : start(std::chrono::high_resolution_clock::now()) {}

    double elapsed_secs() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end - start).count();
    }
};

static void emit_csv(const std::string& benchmark, const std::string& input,
                     const std::string& impl, int threads, int run,
                     double secs, uint64_t checksum) {
    std::cout << benchmark << "," << input << "," << impl << ","
              << threads << "," << run << ","
              << std::fixed << secs << ","
              << checksum << "\n";
}

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
            if (cdf[mid] <= sample) lo = mid + 1;
            else hi = mid;
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

static std::vector<std::string> generate_strings(size_t n, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_int_distribution<int> len_dist(8, 64);
    const char chars[] = "abcdefghijklmnopqrstuvwxyz0123456789";
    std::uniform_int_distribution<int> char_dist(0, 35);

    std::vector<std::string> data(n);
    for (auto& value : data) {
        int len = len_dist(rng);
        value.resize(len);
        for (int i = 0; i < len; i++) {
            value[i] = chars[char_dist(rng)];
        }
    }
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

static uint64_t hash_bytes(const std::string& value) {
    uint64_t hash = FNV_OFFSET;
    for (unsigned char byte : value) {
        hash ^= static_cast<uint64_t>(byte);
        hash *= FNV_PRIME;
    }
    return mix64(hash ^ static_cast<uint64_t>(value.size()));
}

static std::vector<uint64_t> load_data_u64(const std::string& path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        std::cerr << "failed to open: " << path << "\n";
        std::exit(1);
    }
    fin.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(fin.tellg());
    fin.seekg(0);
    std::vector<uint64_t> data(size / 8);
    fin.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    return data;
}

static std::vector<uint32_t> load_data_u32(const std::string& path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        std::cerr << "failed to open: " << path << "\n";
        std::exit(1);
    }
    fin.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(fin.tellg());
    fin.seekg(0);
    std::vector<uint32_t> data(size / 4);
    fin.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    return data;
}

static std::vector<double> load_data_f64(const std::string& path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        std::cerr << "failed to open: " << path << "\n";
        std::exit(1);
    }
    fin.seekg(0, std::ios::end);
    size_t size = static_cast<size_t>(fin.tellg());
    fin.seekg(0);
    std::vector<double> data(size / 8);
    fin.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(size));
    return data;
}

static std::vector<std::string> load_data_strings(const std::string& path) {
    std::ifstream fin(path, std::ios::binary);
    if (!fin) {
        std::cerr << "failed to open: " << path << "\n";
        std::exit(1);
    }

    uint64_t count = 0;
    fin.read(reinterpret_cast<char*>(&count), sizeof(count));

    std::vector<std::string> data;
    data.reserve(static_cast<size_t>(count));
    for (uint64_t i = 0; i < count; i++) {
        uint64_t len = 0;
        fin.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string value(static_cast<size_t>(len), '\0');
        fin.read(value.data(), static_cast<std::streamsize>(len));
        data.push_back(std::move(value));
    }

    return data;
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

static uint64_t checksum_sum_strings(const std::vector<std::string>& data) {
    uint64_t sum = 0;
    for (const auto& value : data) sum += hash_bytes(value);
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

static Graph load_graph(const std::string& path) {
    std::ifstream fin(path);
    if (!fin) {
        std::cerr << "failed to read graph file: " << path << "\n";
        std::exit(1);
    }

    size_t num_nodes;
    size_t num_edges;
    fin >> num_nodes >> num_edges;

    std::vector<size_t> offsets(num_nodes + 1);
    for (size_t i = 0; i < num_nodes; i++) {
        fin >> offsets[i];
    }
    offsets[num_nodes] = num_edges;

    std::vector<uint32_t> edges(num_edges);
    for (size_t i = 0; i < num_edges; i++) {
        fin >> edges[i];
    }

    return Graph{std::move(offsets), std::move(edges), num_nodes};
}
