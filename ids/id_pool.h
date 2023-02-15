// A struct and some functions related to an "IdPool"; an
// unordered set of IDs.

#ifndef ID_POOL_H
#define ID_POOL_H
#include "id.h"
#include "../tools/mem_tools.h"
#include "../tools/byte.h"

struct IdPool
{
        // The number of IDs in the pool.
        size_t len;
        // The number of IDs allocated for the pool.
        size_t capacity;
        // The IDs of the pool; the order is arbitrary.
        // Guaranteed to be packed, so iterating through "contents"
        // from index 0 until and including index "len" - 1" will
        // give you all the IDs in the pool.
        // Adding or removing IDs while this is going on is illegal.
        pcecs_id_t * contents;
        // One bit for each ID, being on iff the ID is in the pool.
        byte_t * id_in_pool;
        pcecs_id_t max_id;
};

// Create an "IdPool", initialized with no IDs.
struct IdPool create_id_pool(void);

// Destroy an ID pool. The IDs themselves aren't invalidated.
void destroy_id_pool(struct IdPool * pool);

// Returns "true" if "pool" contains "value".
bool id_pool_contains(const struct IdPool * pool, pcecs_id_t value);

// Remove an arbitrary ID from a pool of IDs and return its value.
// May invalidate previously assigned pointers to values within
// the pool.
pcecs_id_t steal_from_id_pool(struct IdPool * pool);

// Add "value" to "id_pool". May invalidate previously assigned
// pointers to IDs within the pool.
void add_to_id_pool(struct IdPool * id_pool, pcecs_id_t value);

// Remove "value" from "id_pool", assuming it's there.
// It's not very fast, with a time complexity of O(n).
void remove_from_id_pool(struct IdPool * id_pool, pcecs_id_t value);

// Returns "true" iff "id" is in "id_pool".
// Time complexity O(1).
bool id_in_pool(const struct IdPool * id_pool, pcecs_id_t id);

#endif
