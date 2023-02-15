// Table that organizes the components of entities belonging to the
// same archetype.
// The table maps component types to columns, and entities are mapped
// to indices within those columns (rows, in other words).

#ifndef CTABLE_H
#define CTABLE_H

#include "column.h"
#include "../interface/ct_set.h"
#include "../interface/entity.h"
#include "../ids/id_pool.h"
#include "map.h"

#define CTABLE_FS "component table (%d, %d)"
#define CTABLE_FA(table) (table).ct_to_col.length, (table).row_count

// The maximum possible amount of rows cannot be more than the
// maximum amount of entities, which cannot be more than the maximum
// amount of unique IDs.
typedef pcecs_id_t row_idx_t;

struct CTable {
        // Map component type IDs to "struct Column"s.
        struct Map ct_to_col;

        row_idx_t row_count;

        row_idx_t * entity_to_row_idx;
        row_idx_t entity_to_row_size;

        struct Entity * row_idx_to_entity;
        row_idx_t rows_capacity;

        // When we want to destroy, remove or move elements of a component
        // table while iterating through it (id est when we're executing a
        // system), "row_idx_skipped" will be used to know which elements
        // are no longer in the table, and should therefore not be iterated.
        // "removed_entities" is an unordered set of entity IDs that are
        // supposed to be removed from the "CTable", which is done after the
        // iteration is finished.
        // "destroyed_entities" are the entities that should be removed AND
        // destroyed.
        // Adding entities to the table doesn't impose such difficulties as
        // they're simply appended to the end, so the worst thing that can
        // happen is that they get skipped the first iteration they're in
        // the table.
        // Iteration happens bakcwards! Noitareti!
        bool * row_idx_skipped;
        struct IdPool removed_entities;
        struct IdPool destroyed_entities;

        // If the table is being iterated through, this entity is the one
        // currently being iterated.
        // If the table is not being iterated through, "iterator"'s ID ==
        // "PCECS_INVALID_ID".
        struct Entity iterator;
};

// Create a new component table with all the component types in "cts",
// but no entities.
struct CTable create_ctable(const struct CtSet * cts);

// Add "entity" to table, provided that "entity" belongs to "table"'s
// archetype.
// "entity"'s components will contain junk data.
void add_entity_to_table(struct CTable * table, struct Entity entity);

// Get the component belonging to "entity" of type "ct", provided
// "entity" is in "table".
void * get_table_component(const struct CTable * table, struct Entity entity, struct Ct ct);

// Returns "true" iff "table" is being iterated through using
// "first_entity_in_ctable" and "next_entity_in_ctable".
bool ctable_being_iterated(const struct CTable * table);

// Destroy "entity"'s components and remove it from "table".
void destroy_table_entity(struct CTable * table, struct Entity entity);

// Copy "entity" and its components from "src" to "dest" and remove
// it from "src".
void move_entity(struct CTable * dest, struct CTable * src, struct Entity entity);

// Get an arbitrary "first" entity in "ctable". Use together with
// "next_entity_in_ctable" to iterate through a "CTable".
struct Entity first_entity_in_ctable(struct CTable * ctable);

// Can only be called if "ctable" is currently being iterated through.
// It will stop the iteration of "ctable", so that iteration can
// start from the beginning again next time.
void halt_ctable_iteration(struct CTable * ctable);

// Get an arbitrary 'next' entity in "ctable", 'after' "curr_entity".
// Returns an entity with ID == "PCECS_INVALID_ID" if the end is
// reached.
// "CTables" can only be iterated through by one source at a time,
// and the whole table must be iterated.
struct Entity next_entity_in_ctable(struct CTable * ctable, struct Entity curr_entity);

#endif
