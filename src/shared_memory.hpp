#ifndef OCTO_BLOOM_SHARED_MEMORY_HPP
#define OCTO_BLOOM_SHARED_MEMORY_HPP

extern "C" {
#include <postgres.h>
#include <fmgr.h>
#include <storage/lwlock.h>
#include <storage/shmem.h>
#include <utils/hsearch.h>
#include <utils/builtins.h>
#include <utils/lsyscache.h>
#include <utils/rel.h>
#include <common/hashfn.h>
#include <catalog/pg_type.h>
#include <executor/spi.h>
#include <lib/stringinfo.h>
#include <utils/elog.h>
}

#include "bloom_filter.hpp"

typedef struct BloomRegistryEntry {
    Oid table_oid;
    int16_t attnum;
    OctoBloomFilter* filter;
    LWLock* lock;
    uint64_t expected_count;
    double false_positive_rate;
    uint64_t current_count;
    bool is_valid;
} BloomRegistryEntry;

typedef struct BloomSharedState {
    HTAB* bloom_registry;
    LWLock* registry_lock;
    Size total_memory;
    Size used_memory;
    int max_filters;
} BloomSharedState;

// Global shared state pointer
extern "C" {
extern BloomSharedState* bloom_shared_state;
}

// Function prototypes
extern "C" {
void init_shared_memory();
OctoBloomFilter* get_bloom_filter(Oid table_oid, int16_t attnum);
bool register_bloom_filter(Oid table_oid, int16_t attnum,
                          uint64_t expected_count, double false_positive_rate);
void unregister_bloom_filter(Oid table_oid, int16_t attnum);
Size calculate_shared_memory_size(int max_filters, Size filter_memory);
}

#endif // OCTO_BLOOM_SHARED_MEMORY_HPP