// Unordered map mapping IDs to generic data.
// IDs are used as indices in a special data structure, so access
// is quite fast.

#ifndef MAP_H
#define MAP_H

#include <stdbool.h>
#include <stddef.h>
#include "../ids/id.h"

#define MAP_FS "map (%d elements)"
#define MAP_FA(map) (map).length

typedef unsigned int map_idx_t;

struct Map {
        map_idx_t * id_to_index;
        size_t id_to_index_size;

        pcecs_id_t * index_to_id;
        size_t value_size;
        void * values;
        // Measured in the amount of "value"s, not bytes.
        map_idx_t values_capacity;

        map_idx_t length;

        void (* value_destructor)(void * value);
};

// Create a map structure with no elements.
// "value_size" is the size of a single map element.
// "value_destructor" destroys a map value, passed as a void pointer.
struct Map create_map(size_t value_size, void (* value_destructor)(void * value));

// Returns whether or not "map" can map "id" to anything.
bool map_contains(const struct Map * map, pcecs_id_t id);

// Free the resources allocated by "map" and destroy its values using
// the value destructor it's initialized with.
void destroy_map(struct Map * map);

// Map "id" to an element within "map".
// Returns "NULL" if "map" does not contain "id".
void * get_map_element_nullable(const struct Map * map, pcecs_id_t id);

// Same as "get_map_element_nullable", except it cannot be called if
// "map" doesn't contain "id".
void * get_map_element(const struct Map * map, pcecs_id_t id);

// Map "id" to "value", provided "id" isn't already in "map".
void add_to_map(struct Map * map, pcecs_id_t id, void * value);

// Remove "id" and the value it maps to, and destroy the value
// using the value destructor "map" is initialized with.
void remove_from_map(struct Map * map, pcecs_id_t id);

#endif
