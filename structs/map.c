#include "map.h"
#include "../tools/mem_tools.h"
#include "../tools/byte.h"

// Assuming integer types where all bits are 1 are:
// *    Valid
// *    Either very large or negative.
#define INVALID_MAP_IDX (~((map_idx_t) 0))

#define VALS_CAPACITY_MUL 2

// The minimum valid capacity for the values stored in a map greater
// than or equal to "req_capacity".
static map_idx_t min_valid_values_capacity(map_idx_t req_capacity)
{
        map_idx_t capacity = 1;
        while (capacity < req_capacity) {
                capacity *= VALS_CAPACITY_MUL;
        }
        return capacity;
}

static void noop(void * arg)
{
        (void) arg;
}

struct Map create_map(size_t value_size, void (* value_destructor)(void * value))
{
        LOG_DEBUG("Creating map with value size %d ...\n", (int) value_size);

        struct Map map;

        // If the initial size of "id_to_index" changes, its values must be
        // initialized to "INVALID_MAP_IDX" to mark the IDs as unused.
        map.id_to_index_size = 0;
        map.id_to_index = ALLOC(map_idx_t, map.id_to_index_size);

        map.value_size = value_size;
        map.length = 0;
        map.values_capacity = min_valid_values_capacity(map.length);
        // These values does not need to be initialized to invalid values like
        // "id_to_index", since the length of the map is initialized to 0,
        // making it clear that none of these values are actually used.
        map.index_to_id = ALLOC(pcecs_id_t, map.values_capacity);

        map.values = ALLOC(byte_t, map.value_size * map.values_capacity);

        // If a value destructor is not provided, values will be removed from
        // the map without being destroyed.
        map.value_destructor = value_destructor ? value_destructor : noop;

        LOG_DEBUG("Created " MAP_FS ".\n", MAP_FA(map));
        return map;
}

#ifdef ASSERTIONS
        static void validate_map(const struct Map * map)
        {
                ASSERT(map->values_capacity >= map->length, "Capacity %d not enough for length %d.",
                        (int) map->values_capacity, (int) map->length);
                ASSERT(map->value_destructor, "No value destructor for " MAP_FS ".", MAP_FA(*map));
        }
#else
        #define validate_map(map)
#endif

void destroy_map(struct Map * map)
{
        LOG_DEBUG("Destroying " MAP_FS " ...\n", MAP_FA(*map));

        // Destroy each value.
        for (int i = 0; i < map->length; ++i) {
                void * value = (byte_t *) map->values + map->value_size * i;
                map->value_destructor(value);
        }
        // Free resources.
        FREE(map->id_to_index);
        FREE(map->index_to_id);
        FREE(map->values);
}

bool map_contains(const struct Map * map, pcecs_id_t id)
{
        // IDs in "map" are guaranteed to be between 0 and "id_to_index_size" - 1,
        // so an ID greater than that is not in the map.
        if (id >= map->id_to_index_size) {
                return false;
        }

        // Return whether "id" is marked as unused.
        return map->id_to_index[id] != INVALID_MAP_IDX;
}

void * get_map_element_nullable(const struct Map * map, pcecs_id_t id)
{
        validate_map(map);

        if (!map_contains(map, id)) {
                return NULL;
        }

        size_t index = map->id_to_index[id];
        // The offset from the beginning of map->values equals the index times
        // the size of a single map element.
        // "sizeof(void)" not standard, so "byte_t"s are used for arithmetics.
        return (byte_t *) map->values + map->value_size * index;
}

void * get_map_element(const struct Map * map, pcecs_id_t id)
{
        ASSERT(map_contains(map, id), PCECS_ID_FS " not in " MAP_FS ".",
                PCECS_ID_FA(id), MAP_FA(*map));

        return get_map_element_nullable(map, id);
}

static void resize_id_to_index(struct Map * map, size_t size)
{
        // TODO: fix problem with this realloc statement. sometimes, it gives an error when
        // reallocing, sometimes it's segfault
        REALLOC(&map->id_to_index, map_idx_t, size);

        // If new IDs were allocated, mark them as unused by "map".
        if (size > map->id_to_index_size) {
                for (size_t i = map->id_to_index_size; i < size; ++i) {
                        map->id_to_index[i] = INVALID_MAP_IDX;
                }
        }

        map->id_to_index_size = size;
}

static void set_values_capacity(struct Map * map, map_idx_t capacity)
{
        map_idx_t valid_capacity = min_valid_values_capacity(capacity);

        // "values" and "index_to_id" are resized together since they
        // both have the same length.
        REALLOC(&map->values, byte_t, map->value_size * valid_capacity);
        REALLOC(&map->index_to_id, pcecs_id_t, valid_capacity);

        map->values_capacity = valid_capacity;
}

static void set_map_length(struct Map * map, map_idx_t length)
{
        // Reallocate if "length" is too large (or way too little) for
        // the capacity of "map".
        if (length > map->values_capacity ||
                length * VALS_CAPACITY_MUL <= map->values_capacity) {

                set_values_capacity(map, length);
        }
        map->length = length;
}

void add_to_map(struct Map * map, pcecs_id_t id, void * value)
{
        LOG_DEBUG("Adding " PCECS_ID_FS " to " MAP_FS " ...\n", PCECS_ID_FA(id), MAP_FA(*map));

        ASSERT(!map_contains(map, id),
                "Cannot add " PCECS_ID_FS " to " MAP_FS " as it's already there.",
                PCECS_ID_FA(id), MAP_FA(*map));

        if (map->id_to_index_size <= id) {
                resize_id_to_index(map, id + 1);
        }

        set_map_length(map, map->length + 1);

        // Copy "value" to the newly allocated space in "map".
        void * new_element = (byte_t *) map->values + map->value_size * (map->length - 1);
        COPY_MEMORY(new_element, value, byte_t, map->value_size);

        // Map the last element of "map" to "id" and vice versa.
        map->index_to_id[map->length - 1] = id;
        map->id_to_index[id] = map->length - 1;
}

void remove_from_map(struct Map * map, pcecs_id_t id)
{
        LOG_DEBUG("Removing " PCECS_ID_FS " from " MAP_FS " ...\n",
                PCECS_ID_FA(id), MAP_FA(*map));

        ASSERT(map_contains(map, id), "No " PCECS_ID_FS " in " MAP_FS ".",
                PCECS_ID_FA(id), MAP_FA(*map));

        // Destroy the value belonging to "id".
        map_idx_t destroyed_index = map->id_to_index[id];
        void * destroyed_value = (byte_t *) map->values + map->value_size * destroyed_index;
        map->value_destructor(destroyed_value);

        // Mark "id" as unused by the map.
        map->id_to_index[id] = INVALID_MAP_IDX;

        // Unless the destroyed element is the last value in "map", move
        // the last value to where the destroyed element was and remap
        // the ID of the moved element to that index, and remap that
        // index to the ID.
        if (destroyed_index != map->length - 1) {
                void * last_value = (byte_t *) map->values + map->value_size * (map->length - 1);
                COPY_MEMORY(destroyed_value, last_value, byte_t, map->value_size);

                pcecs_id_t last_value_id = map->index_to_id[map->length - 1];
                map->id_to_index[last_value_id] = destroyed_index;
                map->index_to_id[destroyed_index] = last_value_id;
        }

        // Now that the last element of "map" is copied to an earlier
        // index, remove the last copy of it.
        set_map_length(map, map->length - 1);
}
