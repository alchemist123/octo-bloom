#ifndef OCTO_BLOOM_BLOOM_FILTER_HPP
#define OCTO_BLOOM_BLOOM_FILTER_HPP

#include <vector>
#include <cstdint>
#include <cmath>
#include <functional>
#include <cstring>

class OctoBloomFilter {
public:
    OctoBloomFilter(uint64_t expected_count, double false_positive_rate);
    ~OctoBloomFilter() = default;

    // Disallow copying
    OctoBloomFilter(const OctoBloomFilter&) = delete;
    OctoBloomFilter& operator=(const OctoBloomFilter&) = delete;

    void add(const void* data, size_t length);
    bool mightContain(const void* data, size_t length) const;
    void remove(const void* data, size_t length); // For counting Bloom filter
    void clear();
    
    size_t getMemoryUsage() const;
    uint64_t getExpectedCount() const { return expected_count_; }
    double getFalsePositiveRate() const { return false_positive_rate_; }
    size_t getBitArraySize() const { return bit_array_size_; }
    uint32_t getNumHashes() const { return num_hashes_; }

    // Serialization methods
    size_t getSerializedSize() const;
    void serialize(uint8_t* buffer) const;
    bool deserialize(const uint8_t* buffer, size_t size);

private:
    uint8_t* bits_;  // Bit array stored as bytes
    uint32_t num_hashes_;
    uint64_t expected_count_;
    double false_positive_rate_;
    size_t bit_array_size_;
    size_t byte_array_size_;  // Size in bytes

    // Double hashing implementation
    std::pair<uint64_t, uint64_t> doubleHash(const void* data, size_t length) const;
    uint64_t hash1(const void* data, size_t length) const;
    uint64_t hash2(const void* data, size_t length) const;
    
    // Hash function using PostgreSQL's hash functions
    uint64_t postgresHash(const void* data, size_t length) const;
    uint64_t simple_hash(const void* data, size_t length) const;
};

#endif // OCTO_BLOOM_BLOOM_FILTER_HPP