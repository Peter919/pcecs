#include "id_pool.h"

#define ID_POOL_CAPACITY_MUL (2)

struct IdPool create_id_pool(void)
{
        struct IdPool pool;
        pool.len = 0;
        // Changes in the capacity of "struct IdPool"s are
        // made through multiplication, so initializing the
        // capacity to 0 would cause infinite loops when it's
        // multiplied over and over to grow.
        pool.capacity = 1;
        pool.contents = ALLOC(pcecs_id_t, pool.capacity);
        pool.id_in_pool = ALLOC(byte_t, 0);
        pool.max_id = 0;
        return pool;
}

#ifdef ASSERTIONS
static bool valid_capacity(size_t capacity, size_t len)
{
        // Again, changes in capacity is done through multiplication
        // and division, so a capacity of 0 would create problems if
        // a routine tries to multiply it to make it grow.
        if (capacity == 0) {
                return false;
        }
        if (capacity < len) {
                return false;
        }

        // Here, we could also check if capacity is too high
        // for len and should have been reduced. However, that
        // rule is temporarily broken in several routines
        // handling resizing, so ignoring the potential problem
        // might be a better solution.

        return true;
}
#endif

#ifdef ASSERTIONS
static void validate_id_pool(const struct IdPool * pool)
{
        ASSERT(valid_capacity(pool->capacity, pool->len),
                "Capacity %d not valid for length %d.",
                (int) pool->capacity, (int) pool->len);
}
#else
        #define validate_id_pool(pool)
#endif

void destroy_id_pool(struct IdPool * pool)
{
        validate_id_pool(pool);
        FREE(pool->contents);
        FREE(pool->id_in_pool);
}

static void resize_id_pool(struct IdPool * pool, size_t capacity)
{
        LOG_DEBUG("Resizing id pool from capacity %d to %d ...\n",
                (int) pool->capacity, (int) capacity);

        ASSERT(valid_capacity(capacity, pool->len), "New capacity %d not valid for length %d.",
                (int) capacity, (int) pool->len);

        REALLOC(&pool->contents, pcecs_id_t, capacity);

        pool->capacity = capacity;
}

static void set_id_pool_len(struct IdPool * pool, size_t len)
{
        LOG_DEBUG("Changing the length of an id pool from %d to %d ...\n",
                (int) pool->len, (int) len);

        validate_id_pool(pool);

        if (len > pool->capacity) {
                // Multiply capacity until it's large enough for the new length.
                size_t new_capacity = pool->capacity;
                do {
                        new_capacity *= ID_POOL_CAPACITY_MUL;
                } while (new_capacity < len);

                resize_id_pool(pool, new_capacity);
        }

        pool->len = len;

        // Shrink the id pool if its capacity is far greater
        // than it needs to be. The minimum valid capacity is
        // 1 since the resizing is done through multiplication
        // and division, and multiplying or dividing 0 by anything
        // yields a result of 0.
        if (pool->capacity != 1 &&
            pool->capacity >= pool->len * ID_POOL_CAPACITY_MUL)
        {
                size_t new_capacity = pool->capacity;
                do {
                        new_capacity /= ID_POOL_CAPACITY_MUL;
                } while (new_capacity >= pool->len * ID_POOL_CAPACITY_MUL);

                resize_id_pool(pool, new_capacity);
        }
}

// TODO: comment all below

bool id_pool_contains(const struct IdPool * pool, pcecs_id_t value)
{
        for (pcecs_id_t i = 0; i < pool->len; ++i) {
                if (pool->contents[i] == value) {
                        return true;
                }
        }
        return false;
}

static byte_t idx_of_byte(pcecs_id_t id)
{
        return id / CHAR_BIT;
}

static byte_t idx_of_bit_within_byte(pcecs_id_t id)
{
        return id % CHAR_BIT;
}

static void remove_idx_from_id_pool(struct IdPool * pool, size_t idx)
{
        pcecs_id_t removed_id = pool->contents[idx];
        pool->contents[idx] = pool->contents[pool->len - 1];

        set_id_pool_len(pool, pool->len - 1);

        byte_t * byte_with_value = &pool->id_in_pool[idx_of_byte(removed_id)];
        byte_t bit_with_value = 1 << idx_of_bit_within_byte(removed_id);
        *byte_with_value &= ~bit_with_value;
}

pcecs_id_t steal_from_id_pool(struct IdPool * pool)
{
        ASSERT(pool->len != 0, "Cannot steal from an empty pool.");

        pcecs_id_t removed_id = pool->contents[pool->len - 1];
        remove_idx_from_id_pool(pool, pool->len - 1);

        return removed_id;
}

static void set_max_id(struct IdPool * id_pool, pcecs_id_t new_max_id)
{
        REALLOC(&id_pool->id_in_pool, byte_t, idx_of_byte(new_max_id) + 1);

        if (new_max_id > id_pool->max_id) {
                for (int i = idx_of_byte(id_pool->max_id) + 1; i <= idx_of_byte(new_max_id); ++i) {
                        id_pool->id_in_pool[i] = 0;
                }
        }

        id_pool->max_id = new_max_id;
}

void add_to_id_pool(struct IdPool * id_pool, pcecs_id_t value)
{
        ASSERT(!id_pool_contains(id_pool, value), "Id already in pool.");

        // Add a new element to the pool and set its value.
        set_id_pool_len(id_pool, id_pool->len + 1);
        id_pool->contents[id_pool->len - 1] = value;

        if (value > id_pool->max_id) {
                set_max_id(id_pool, value);
        }

        byte_t * byte_with_value = &id_pool->id_in_pool[idx_of_byte(value)];
        byte_t bit_with_value = 1 << idx_of_bit_within_byte(value);
        *byte_with_value |= bit_with_value;
}

// Replace "value" in "id_pool" with the last ID and remove the last
// ID.
void remove_from_id_pool(struct IdPool * id_pool, pcecs_id_t value)
{
        bool found = false;
        // Unused if "ASSERTIONS" isn't defined.
        (void) found;

        size_t idx_of_id = 0;

        for (size_t i = 0; i < id_pool->len; ++i) {
                if (id_pool->contents[i] == value) {
                        idx_of_id = i;
                        found = true;
                        break;
                }
        }

        ASSERT(found, "Id not in pool.");

        remove_idx_from_id_pool(id_pool, idx_of_id);
}

bool id_in_pool(const struct IdPool * id_pool, pcecs_id_t id)
{
        if (id > id_pool->max_id) {
                return false;
        }

        byte_t * byte_with_id = &id_pool->id_in_pool[idx_of_byte(id)];
        byte_t bit_with_id = 1 << idx_of_bit_within_byte(id);
        return *byte_with_id & bit_with_id;
}
