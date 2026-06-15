#pragma once

#include <bit>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

static constexpr uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
static constexpr uint64_t FNV_PRIME = 0x100000001b3ULL;
static constexpr const char* BENCHRC_DATASETS_ENV = "BENCHRC_DATASETS_DIR";

static std::filesystem::path datasets_dir() {
    if (const char* value = std::getenv(BENCHRC_DATASETS_ENV)) {
        return std::filesystem::path(value);
    }
    return std::filesystem::path("datasets");
}

static std::filesystem::path dataset_path(const std::string& name) {
    return datasets_dir() / name;
}

static std::vector<char> read_dataset_bytes(const std::string& name) {
    auto path = dataset_path(name);
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) {
        throw std::runtime_error(
            "failed to read dataset " + name + " at " + path.string() +
            ". run `python3 scripts/generate_datasets.py` first");
    }

    auto size = static_cast<size_t>(input.tellg());
    input.seekg(0);

    std::vector<char> bytes(size);
    input.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
    if (!input) {
        throw std::runtime_error("failed to read full dataset " + name + " from " + path.string());
    }
    return bytes;
}

static uint32_t read_u32_le(const char* ptr) {
    return static_cast<uint32_t>(static_cast<unsigned char>(ptr[0])) |
           (static_cast<uint32_t>(static_cast<unsigned char>(ptr[1])) << 8) |
           (static_cast<uint32_t>(static_cast<unsigned char>(ptr[2])) << 16) |
           (static_cast<uint32_t>(static_cast<unsigned char>(ptr[3])) << 24);
}

static uint64_t read_u64_le(const char* ptr) {
    return static_cast<uint64_t>(static_cast<unsigned char>(ptr[0])) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[1])) << 8) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[2])) << 16) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[3])) << 24) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[4])) << 32) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[5])) << 40) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[6])) << 48) |
           (static_cast<uint64_t>(static_cast<unsigned char>(ptr[7])) << 56);
}

static std::vector<uint64_t> load_u64_dataset(const std::string& name, size_t expected_len) {
    auto bytes = read_dataset_bytes(name);
    size_t expected_bytes = expected_len * sizeof(uint64_t);
    if (bytes.size() != expected_bytes) {
        throw std::runtime_error("dataset " + name + " has unexpected size");
    }

    std::vector<uint64_t> values;
    values.reserve(expected_len);
    for (size_t offset = 0; offset < bytes.size(); offset += 8) {
        values.push_back(read_u64_le(bytes.data() + offset));
    }
    return values;
}

static std::vector<uint32_t> load_u32_dataset(const std::string& name, size_t expected_len) {
    auto bytes = read_dataset_bytes(name);
    size_t expected_bytes = expected_len * sizeof(uint32_t);
    if (bytes.size() != expected_bytes) {
        throw std::runtime_error("dataset " + name + " has unexpected size");
    }

    std::vector<uint32_t> values;
    values.reserve(expected_len);
    for (size_t offset = 0; offset < bytes.size(); offset += 4) {
        values.push_back(read_u32_le(bytes.data() + offset));
    }
    return values;
}

static std::vector<double> load_f64_dataset(const std::string& name, size_t expected_len) {
    auto bytes = read_dataset_bytes(name);
    size_t expected_bytes = expected_len * sizeof(double);
    if (bytes.size() != expected_bytes) {
        throw std::runtime_error("dataset " + name + " has unexpected size");
    }

    std::vector<double> values;
    values.reserve(expected_len);
    for (size_t offset = 0; offset < bytes.size(); offset += 8) {
        values.push_back(std::bit_cast<double>(read_u64_le(bytes.data() + offset)));
    }
    return values;
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

static uint64_t checksum_ordered_u64(const std::vector<uint64_t>& data) {
    uint64_t hash = FNV_OFFSET;
    for (size_t index = 0; index < data.size(); index++) {
        hash ^= mix64(data[index] ^ static_cast<uint64_t>(index));
        hash *= FNV_PRIME;
    }
    return hash;
}

static uint64_t checksum_sum_u32(const std::vector<uint32_t>& data) {
    uint64_t sum = 0;
    for (auto value : data) sum += mix64(static_cast<uint64_t>(value));
    return sum;
}

static uint64_t checksum_ordered_u32(const std::vector<uint32_t>& data) {
    uint64_t hash = FNV_OFFSET;
    for (size_t index = 0; index < data.size(); index++) {
        hash ^= mix64(static_cast<uint64_t>(data[index]) ^ static_cast<uint64_t>(index));
        hash *= FNV_PRIME;
    }
    return hash;
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

static Graph load_graph_dataset(const std::string& name) {
    auto bytes = read_dataset_bytes(name);
    if (bytes.size() < 24) {
        throw std::runtime_error("graph dataset " + name + " is too small");
    }

    size_t num_nodes = static_cast<size_t>(read_u64_le(bytes.data()));
    size_t offsets_len = static_cast<size_t>(read_u64_le(bytes.data() + 8));
    size_t edges_len = static_cast<size_t>(read_u64_le(bytes.data() + 16));

    size_t expected_bytes = 24 + offsets_len * sizeof(uint64_t) + edges_len * sizeof(uint32_t);
    if (bytes.size() != expected_bytes) {
        throw std::runtime_error("graph dataset " + name + " has unexpected size");
    }

    Graph graph;
    graph.num_nodes = num_nodes;
    graph.offsets.reserve(offsets_len);
    graph.edges.reserve(edges_len);

    size_t offset = 24;
    for (size_t i = 0; i < offsets_len; i++, offset += 8) {
        graph.offsets.push_back(static_cast<size_t>(read_u64_le(bytes.data() + offset)));
    }
    for (size_t i = 0; i < edges_len; i++, offset += 4) {
        graph.edges.push_back(read_u32_le(bytes.data() + offset));
    }

    return graph;
}
