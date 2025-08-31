#include "bloom_filter.hpp"
#include <openssl/sha.h>
#include <algorithm>
#include <stdexcept>

extern "C" {
#include <postgres.h>
#include <common/hashfn.h>
#include <utils/fmgrprotos.h>
#include <utils/palloc.h>
}

OctoBloomFilter::OctoBloomFilter(uint64_t expected_count, double false_positive_rate)
    : expected_count_(expected_count),
      false_positive_rate_(false_positive_rate) {
    
    // Parameters should be validated before calling constructor

    // Calculate optimal parameters
    bit_array_size_ = static_cast<size_t>(
        - (expected_count * std::log(false_positive_rate)) / std::pow(std::log(2), 2)
    );
    
    num_hashes_ = static_cast<uint32_t>(
        std::round((static_cast<double>(bit_array_size_) / expected_count) * std::log(2))
    );

    // Ensure minimum values
    bit_array_size_ = std::max(bit_array_size_, static_cast<size_t>(64));
    num_hashes_ = std::max(num_hashes_, 1u);
    num_hashes_ = std::min(num_hashes_, 50u); // Reasonable upper limit

    // Calculate byte array size and allocate memory
    byte_array_size_ = (bit_array_size_ + 7) / 8; // Round up to bytes
    bits_ = (uint8_t*)palloc(byte_array_size_);
    memset(bits_, 0, byte_array_size_);
}

void OctoBloomFilter::add(const void* data, size_t length) {
    auto hashes = doubleHash(data, length);
    uint64_t h1 = hashes.first;
    uint64_t h2 = hashes.second;

    for (uint32_t i = 0; i < num_hashes_; ++i) {
        uint64_t hash = h1 + i * h2;
        size_t index = hash % bit_array_size_;
        size_t byte_index = index / 8;
        uint8_t bit_mask = 1 << (index % 8);
        bits_[byte_index] |= bit_mask;
    }
}

bool OctoBloomFilter::mightContain(const void* data, size_t length) const {
    auto hashes = doubleHash(data, length);
    uint64_t h1 = hashes.first;
    uint64_t h2 = hashes.second;

    for (uint32_t i = 0; i < num_hashes_; ++i) {
        uint64_t hash = h1 + i * h2;
        size_t index = hash % bit_array_size_;
        size_t byte_index = index / 8;
        uint8_t bit_mask = 1 << (index % 8);
        if (!(bits_[byte_index] & bit_mask)) {
            return false;
        }
    }
    return true;
}

void OctoBloomFilter::remove(const void* data, size_t length) {
    // For counting Bloom filter implementation
    // This is a placeholder - actual implementation would require counting bits
    // For simplicity, we're using a regular Bloom filter in this example
    ereport(WARNING,
            (errmsg("Remove operation not supported for regular Bloom filter")));
}

void OctoBloomFilter::clear() {
    memset(bits_, 0, byte_array_size_);
}

size_t OctoBloomFilter::getMemoryUsage() const {
    return (bit_array_size_ + 7) / 8; // Return size in bytes
}

std::pair<uint64_t, uint64_t> OctoBloomFilter::doubleHash(const void* data, size_t length) const {
    uint64_t h1 = hash1(data, length);
    uint64_t h2 = hash2(data, length);
    return {h1, h2};
}

uint64_t OctoBloomFilter::hash1(const void* data, size_t length) const {
    // Use PostgreSQL's hash function with error checking
    if (!data || length == 0) {
        return 0;
    }
    Datum hash_datum = hash_any((const unsigned char*)data, length);
    if (hash_datum == 0) {
        // Fallback to simple hash if PostgreSQL hash fails
        return simple_hash(data, length);
    }
    return DatumGetUInt64(hash_datum);
}

uint64_t OctoBloomFilter::simple_hash(const void* data, size_t length) const {
    // Simple fallback hash function
    uint64_t hash = 5381;
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < length; ++i) {
        hash = ((hash << 5) + hash) + bytes[i]; // hash * 33 + c
    }
    return hash;
}

uint64_t OctoBloomFilter::hash2(const void* data, size_t length) const {
    // Simple hash function as alternative to SHA256
    if (!data || length == 0) {
        return 1; // Different seed than hash1
    }

    uint64_t hash = 0x9e3779b97f4a7c15; // Different seed
    const unsigned char* bytes = (const unsigned char*)data;
    for (size_t i = 0; i < length; ++i) {
        hash ^= bytes[i];
        hash *= 0x100000001b3; // FNV prime
        hash ^= (hash >> 32);
    }
    return hash;
}

size_t OctoBloomFilter::getSerializedSize() const {
    return sizeof(uint64_t) * 3 + sizeof(uint32_t) + (bit_array_size_ + 7) / 8;
}

void OctoBloomFilter::serialize(uint8_t* buffer) const {
    uint64_t* u64_ptr = reinterpret_cast<uint64_t*>(buffer);
    u64_ptr[0] = expected_count_;
    u64_ptr[1] = bit_array_size_;
    u64_ptr[2] = *reinterpret_cast<const uint64_t*>(&false_positive_rate_);
    
    uint32_t* u32_ptr = reinterpret_cast<uint32_t*>(buffer + sizeof(uint64_t) * 3);
    u32_ptr[0] = num_hashes_;
    
    uint8_t* bits_buffer = buffer + sizeof(uint64_t) * 3 + sizeof(uint32_t);
    memcpy(bits_buffer, bits_, byte_array_size_);
}

bool OctoBloomFilter::deserialize(const uint8_t* buffer, size_t size) {
    if (size < sizeof(uint64_t) * 3 + sizeof(uint32_t)) {
        return false;
    }
    
    const uint64_t* u64_ptr = reinterpret_cast<const uint64_t*>(buffer);
    expected_count_ = u64_ptr[0];
    bit_array_size_ = u64_ptr[1];
    false_positive_rate_ = *reinterpret_cast<const double*>(&u64_ptr[2]);
    
    const uint32_t* u32_ptr = reinterpret_cast<const uint32_t*>(buffer + sizeof(uint64_t) * 3);
    num_hashes_ = u32_ptr[0];
    
    size_t expected_size = sizeof(uint64_t) * 3 + sizeof(uint32_t) + (bit_array_size_ + 7) / 8;
    if (size < expected_size) {
        return false;
    }

    // Allocate memory for bits array
    byte_array_size_ = (bit_array_size_ + 7) / 8;
    bits_ = (uint8_t*)palloc(byte_array_size_);

    const uint8_t* bits_buffer = buffer + sizeof(uint64_t) * 3 + sizeof(uint32_t);
    memcpy(bits_, bits_buffer, byte_array_size_);
    
    return true;
}