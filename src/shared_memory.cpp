#include "shared_memory.hpp"
#include <cstring>

extern "C" {

// Global shared state pointer
BloomSharedState* bloom_shared_state = nullptr;

void init_shared_memory() {
    bool found;
    Size size = calculate_shared_memory_size(10, 64 * 1024); // 10 filters, 64KB each

    bloom_shared_state = (BloomSharedState*)ShmemInitStruct("octo_bloom_shared_state",
                                                          size, &found);

    if (!found) {
        // Initialize shared memory
        memset(bloom_shared_state, 0, size);

        HASHCTL info;
        memset(&info, 0, sizeof(info));
        info.keysize = sizeof(Oid) + sizeof(int16_t);
        info.entrysize = sizeof(BloomRegistryEntry);
        info.hash = tag_hash;

        bloom_shared_state->bloom_registry = ShmemInitHash("octo_bloom_registry",
                                                         10, 10,
                                                         &info,
                                                         HASH_ELEM | HASH_FUNCTION);

        // bloom_shared_state->registry_lock = LWLockAssign(); // TODO: Implement proper locking
        bloom_shared_state->total_memory = size;
        bloom_shared_state->max_filters = 10;
    }
}

OctoBloomFilter* get_bloom_filter(Oid table_oid, int16_t attnum) {
    if (!bloom_shared_state) {
        return nullptr;
    }

    // LWLockAcquire(bloom_shared_state->registry_lock, LW_SHARED); // TODO: Implement proper locking

    // Create hash key
    char key[sizeof(Oid) + sizeof(int16_t)];
    memcpy(key, &table_oid, sizeof(Oid));
    memcpy(key + sizeof(Oid), &attnum, sizeof(int16_t));

    BloomRegistryEntry* entry = (BloomRegistryEntry*)hash_search(
        bloom_shared_state->bloom_registry, key, HASH_FIND, NULL);

    // LWLockRelease(bloom_shared_state->registry_lock); // TODO: Implement proper locking

    if (entry && entry->is_valid) {
        return entry->filter;
    }

    return nullptr;
}

bool register_bloom_filter(Oid table_oid, int16_t attnum,
                          uint64_t expected_count, double false_positive_rate) {
    if (!bloom_shared_state) {
        // Try to initialize shared memory if it's not already done
        init_shared_memory();
        if (!bloom_shared_state) {
            return false;
        }
    }

    // LWLockAcquire(bloom_shared_state->registry_lock, LW_EXCLUSIVE); // TODO: Implement proper locking

    // Create hash key
    char key[sizeof(Oid) + sizeof(int16_t)];
    memcpy(key, &table_oid, sizeof(Oid));
    memcpy(key + sizeof(Oid), &attnum, sizeof(int16_t));

    bool found;
    BloomRegistryEntry* entry = (BloomRegistryEntry*)hash_search(
        bloom_shared_state->bloom_registry, key, HASH_ENTER, &found);

    if (found) {
        // Filter already exists, update it instead
        if (entry->filter) {
            entry->filter->~OctoBloomFilter();
            pfree(entry->filter);
        }
        entry->filter = (OctoBloomFilter*)palloc(sizeof(OctoBloomFilter));
        new (entry->filter) OctoBloomFilter(expected_count, false_positive_rate);
        entry->expected_count = expected_count;
        entry->false_positive_rate = false_positive_rate;
        entry->current_count = 0;
        entry->is_valid = true;
        // LWLockRelease(bloom_shared_state->registry_lock); // TODO: Implement proper locking
        return true;
    }

    // Initialize new entry
    entry->table_oid = table_oid;
    entry->attnum = attnum;
    entry->expected_count = expected_count;
    entry->false_positive_rate = false_positive_rate;
    entry->current_count = 0;
    entry->is_valid = true;
    // entry->lock = LWLockAssign(); // TODO: Implement proper locking

    // Create the bloom filter using PostgreSQL memory management
    entry->filter = (OctoBloomFilter*)palloc(sizeof(OctoBloomFilter));
    new (entry->filter) OctoBloomFilter(expected_count, false_positive_rate);

    // LWLockRelease(bloom_shared_state->registry_lock); // TODO: Implement proper locking
    return true;
}

void unregister_bloom_filter(Oid table_oid, int16_t attnum) {
    if (!bloom_shared_state) {
        return;
    }

    // LWLockAcquire(bloom_shared_state->registry_lock, LW_EXCLUSIVE); // TODO: Implement proper locking

    // Create hash key
    char key[sizeof(Oid) + sizeof(int16_t)];
    memcpy(key, &table_oid, sizeof(Oid));
    memcpy(key + sizeof(Oid), &attnum, sizeof(int16_t));

    BloomRegistryEntry* entry = (BloomRegistryEntry*)hash_search(
        bloom_shared_state->bloom_registry, key, HASH_REMOVE, NULL);

    if (entry && entry->filter) {
        entry->filter->~OctoBloomFilter(); // Call destructor
        pfree(entry->filter);
        entry->filter = nullptr;
    }

    // LWLockRelease(bloom_shared_state->registry_lock); // TODO: Implement proper locking
}

Size calculate_shared_memory_size(int max_filters, Size filter_memory) {
    Size registry_size = hash_estimate_size(max_filters, sizeof(BloomRegistryEntry));
    Size state_size = sizeof(BloomSharedState);
    Size total_filter_memory = max_filters * filter_memory;

    return state_size + registry_size + total_filter_memory;
}

} // extern "C"
