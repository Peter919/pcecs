// TODO: document undocumented documentables

#include "ctable.h"
#include "../tools/log.h"
#include "../interface/entity.h"
#include "ct_data.h"
#include "../globals/maps.h"
#include "../interface/ct.h"

#define BITS_IN_BYTE CHAR_BIT
#define ROWS_CAPACITY_MUL 2

// "row_idx_t" is an integer type. And if I ever encounter an integer type
// where a value of all 1-bits is not super high or negative, I will stop
// programming forever and then it won't matter if this program blows up.
#define INVALID_ROW_IDX (~((row_idx_t) 0))

#define CELL_FS "cell (" CT_FS ", " ENTITY_FS ")"
#define CELL_FA(cell) CT_FA((cell).ct), ENTITY_FA((cell).entity)

struct Cell {
        const struct CTable * table;
        struct Ct ct;
        struct Entity entity;
};

// Find the minimum "valid" capacity for rows greater than or
// equal to "req_capacity".
// By increasing capacity step by step, we don't need to resize
// the every column every time the amount of rows increases.
static size_t min_valid_rows_capacity(size_t req_capacity)
{
        size_t capacity = 1;
        while (capacity < req_capacity) {
                capacity *= ROWS_CAPACITY_MUL;
        }
        return capacity;
}

struct CTable create_ctable(const struct CtSet * cts)
{
        LOG_DEBUG("Creating component table from " CT_SET_FS " ...\n",
                CT_SET_FA(component_types));

        struct CTable table;

        // Initialize component type to column map with no elements.
        // "Column"s are indestructible since "CTable"s are
        // indestructible since "ArctData"s are indestructible since
        // "Arct"s are indestructible. Therefore, we don't
        // provide a destructor for the column map.
        table.ct_to_col = create_map(sizeof(struct Column), NULL);

        // "rows_capacity" is really the capacity of every column, and
        // since there's no rows yet (rows are entities, and a newly
        // created table has no entities), the capacity should be as
        // low as possible.
        table.rows_capacity = min_valid_rows_capacity(0);

        // For each component type that the table should contain,
        // create a column that, in the future, will hold components
        // of that type.
        struct Ct ct = first_ct_in_set(cts);
        while (ct.id != PCECS_INVALID_ID) {

                struct Column col = create_column(ct, table.rows_capacity);
                add_to_map(&table.ct_to_col, ct.id, &col);

                ct = next_ct_in_set(cts, ct);
        }

        table.row_count = 0;

        // If this variable should for some reason not be initialized to 0,
        // the row indices in "table.entity_to_row_idx" should be set to
        // "INVALID_ROW_IDX" to mark that no entity IDs can be mapped to
        // row indices yet.
        table.entity_to_row_size = 0;

        // No "minimum capacity" function is needed for "entity_to_row_idx",
        // as it will be resized exactly as needed and will never shrink
        // since it's far less predictable than simple list (going from
        // 0 to 1 elements might change the capacity from 0 to 100 if the
        // entity has ID 99).
        table.entity_to_row_idx = ALLOC(row_idx_t, table.entity_to_row_size);

        table.row_idx_to_entity = ALLOC(struct Entity, table.rows_capacity);

        table.row_idx_skipped = ALLOC(bool, table.rows_capacity);
        for (row_idx_t i = 0; i < table.rows_capacity; ++i) {
                table.row_idx_skipped[i] = false;
        }

        table.removed_entities = create_id_pool();
        table.destroyed_entities = create_id_pool();

        table.iterator.id = PCECS_INVALID_ID;

        LOG_DEBUG("Created " CTABLE_FS ".\n", CTABLE_FA(table));
        return table;
}

static void set_entity_to_row_size(struct CTable * table, size_t size)
{
        ASSERT(size > table->entity_to_row_size, "Entity row size cannot decrease.");

        REALLOC(&table->entity_to_row_idx, struct Entity, size);

        // For each element in "entity_to_row_idx", initialize it to
        // an invalid row index to show that the entity with ID "i"
        // is not in the table, and therefore does not map to a row
        // index within the table.
        for (row_idx_t i = table->entity_to_row_size; i < size; ++i) {
                table->entity_to_row_idx[i] = INVALID_ROW_IDX;
        }
        table->entity_to_row_size = size;
}

static void set_rows_capacity(struct CTable * table, size_t capacity)
{
        size_t valid_capacity = min_valid_rows_capacity(capacity);

        REALLOC(&table->row_idx_to_entity, struct Entity, valid_capacity);
        REALLOC(&table->row_idx_skipped, bool, valid_capacity);

        // For each column in the component type to column map, resize
        // the column.
        for (map_idx_t i = 0; i < table->ct_to_col.length; ++i) {

                struct Column * col = (struct Column *) table->ct_to_col.values + i;
                resize_column(col, valid_capacity);
        }
}

static void set_row_count(struct CTable * table, size_t count)
{
        table->row_count = count;
        if (table->row_count > table->rows_capacity ||
                table->row_count * ROWS_CAPACITY_MUL <= table->rows_capacity) {

                // Each individual column is resized to change the number
                // of rows that can be accessed (indices within a column
                // are indices of rows).
                set_rows_capacity(table, table->row_count);
        }
}

void add_entity_to_table(struct CTable * table, struct Entity entity)
{
        LOG_DEBUG("Adding " ENTITY_FS " to " CTABLE_FS " ...\n",
                ENTITY_FA(entity), CTABLE_FA(*table));

        // Add a row with junk data (increment the amount of rows
        // without initializing the new row that there's now space
        // for.
        set_row_count(table, table->row_count + 1);

        // Map the index of the new row to "entity".
        table->row_idx_to_entity[table->row_count - 1] = entity;
        table->row_idx_skipped[table->row_count - 1] = false;

        // Map the ID of "entity" to the index of the new row.
        if (entity.id >= table->entity_to_row_size) {
                // Indices start by 0, so accessing an entity requires
                // a size of ID + 1.
                set_entity_to_row_size(table, entity.id + 1);
        }
        table->entity_to_row_idx[entity.id] = table->row_count - 1;
}

static void * get_cell_component(const struct Cell * cell)
{
        // Map the component type of "cell" to a column, and
        // the entity of "cell" to a row.
        struct Column * col = get_map_element(&cell->table->ct_to_col, cell->ct.id);
        row_idx_t row_idx = cell->table->entity_to_row_idx[cell->entity.id];

        // Retrieve a value from that column at the index of the
        // row.
        return (byte_t *) col->components + col->component_size * row_idx;
}

// This function is really just "get_cell_component" except the
// members of the "Cell" struct are given as parameter so we don't
// need to expose that struct externally.
void * get_table_component(const struct CTable * table, struct Entity entity, struct Ct ct)
{
        struct Cell cell = {
                .table = table,
                .entity = entity,
                .ct = ct
        };

        return get_cell_component(&cell);
}

static void destroy_cell_component(const struct Cell * cell)
{
        // Get the component destructor, get the actual component, destroy
        // the component using the destructor.
        const struct CtData * ct_data = get_map_element(&g_ct_map, cell->ct.id);

        void * component = get_cell_component(cell);
        (*ct_data->destructor)(component);
}

static void copy_component(const struct Cell * cell, struct Entity dest_entity)
{
        // Construct a destination cell, get components from the source
        // and the destination, copy the memory from source to destination.
        struct Cell dest_cell = {
                .table = cell->table,
                .ct = cell->ct,
                .entity = dest_entity
        };

        void * src_component = get_cell_component(cell);
        void * dest_component = get_cell_component(&dest_cell);

        struct Column * col = get_map_element(&cell->table->ct_to_col, cell->ct.id);
        COPY_MEMORY(dest_component, src_component, byte_t, col->component_size);
}

static void destroy_col_component(struct CTable * table, struct Entity entity, map_idx_t col_idx)
{
        struct Ct ct = {
                .id = table->ct_to_col.index_to_id[col_idx]
        };

        struct Cell cell = {
                .table = table,
                .ct = ct,
                .entity = entity
        };

        destroy_cell_component(&cell);
}

static void copy_table_entity(const struct CTable * table, struct Entity dest, struct Entity src)
{
        // Loop through each column in the table to duplicate each
        // component of "src" to "dest".
        for (map_idx_t i = 0; i < table->ct_to_col.length; ++i) {

                struct Ct ct;
                ct.id = table->ct_to_col.index_to_id[i];

                struct Cell src_cell = {
                        .table = table,
                        .entity = src,
                        .ct = ct
                };
                copy_component(&src_cell, dest);
        }
}

bool ctable_being_iterated(const struct CTable * table)
{
        return table->iterator.id != PCECS_INVALID_ID;
}

static void mark_entity_as_removed(struct CTable * ctable, struct Entity entity, bool destroyed)
{
        // Add a tag to skip the entity when iterating.
        row_idx_t row_idx = ctable->entity_to_row_idx[entity.id];
        ctable->row_idx_skipped[row_idx] = true;

        // Add the entity to an "IdPool" to make sure it's properly dealt
        // with later (not intended as a threat, although 'dealt with'
        // may in fact refer to murder).
        if (destroyed) {
                add_to_id_pool(&ctable->destroyed_entities, entity.id);
        } else {
                add_to_id_pool(&ctable->removed_entities, entity.id);
        }
}

// This routine isn't good, but at least it's documented :)
static void remove_table_entity(struct CTable * table, struct Entity entity, bool destroy)
{
        LOG_DEBUG("Removing " ENTITY_FS " from " CTABLE_FS " ...\n",
                ENTITY_FA(entity), CTABLE_FA(*table));

        // Removing the entity may partially reorder the table, which
        // would be difficult to handle while iterating.
        // Instead, we avoid the problem by instead marking the entity
        // to be removed later.
        if (ctable_being_iterated(table)) {
                mark_entity_as_removed(table, entity, destroy);
                return;
        }

        if (destroy) {
                // For each column, destroy the component in that column
                // belonging to "entity".
                for (map_idx_t i = 0; i < table->ct_to_col.length; ++i) {
                        destroy_col_component(table, entity, i);
                }
        }

        // Unless the entity to be destroyed is the last entity,
        // copy the last entity to wherever "entity" is stored.
        // If "entity" was the last entity and this was done,
        // memory would be copied to the same location, which
        // is undefined behaviour.
        row_idx_t row_idx = table->entity_to_row_idx[entity.id];
        if (row_idx != table->row_count - 1) {
                // Copy the last entity in the table to the entity that is being
                // removed. Now, we have two instances of "last_entity" in the
                // table, so we can safely remove the last one (which is a lot
                // easier than removing a row in the middle of the table.
                struct Entity last_entity = table->row_idx_to_entity[table->row_count - 1];
                copy_table_entity(table, entity, last_entity);

                // Now that the entity being destroyed is replaced by the last
                // entity, we can map the index of the destroyed entity to the
                // previously last entity and map the ID of the previously last
                // entity to the index of the destroyed one.
                table->entity_to_row_idx[last_entity.id] = row_idx;
                table->row_idx_to_entity[row_idx] = last_entity;
                table->row_idx_skipped[row_idx] = table->row_idx_skipped[table->row_count - 1];
        }

        // Map the destroyed entity ID to an invalid row index to show
        // that the entity is no longer there.
        // We don't need to map the destroyed entity's row index to an
        // invalid ID, since it's already remapped to the previously
        // last entity.
        table->entity_to_row_idx[entity.id] = INVALID_ROW_IDX;

        // Decrease the row count to remove the last entity (the one
        // that has a copy earlier in the table now) unless "entity"
        // is the last entity, in which case "entity" is removed.
        set_row_count(table, table->row_count - 1);
}

void destroy_table_entity(struct CTable * table, struct Entity entity)
{
        LOG_DEBUG("Destroying components of " ENTITY_FS " in " CTABLE_FS " ...\n",
                ENTITY_FA(entity), CTABLE_FA(*table));

        // Remove and destroy.
        remove_table_entity(table, entity, true);
}

#ifdef ASSERTIONS
static bool entity_in_table(struct CTable * table, struct Entity entity)
{
        // If the table hasn't bothered allocating space for the
        // ID of "entity", it's not there.
        if (table->entity_to_row_size <= entity.id) {
                return false;
        }

        row_idx_t row_idx = table->entity_to_row_idx[entity.id];

        // Entities not in the table are mapped to invalid row
        // indices.
        if (row_idx == INVALID_ROW_IDX) {
                return false;
        }

        // When a row (aka entity) is skipped upon iteration, that
        // means that the row is removed from the table.
        if (table->row_idx_skipped[row_idx]) {
                return false;
        }

        return true;
}
#endif

static void copy_cell_component(struct Cell * dest, struct Cell * src)
{
        // Get both components, copy the memory.
        ASSERT(cts_equal(dest->ct, src->ct),
                "Component type mismatch in " CELL_FS " and " CELL_FS ".",
                CELL_FA(*dest), CELL_FA(*src));

        const struct CtData * type_data = get_map_element(&g_ct_map, src->ct.id);

        void * dest_component = get_cell_component(dest);
        void * src_component = get_cell_component(src);

        COPY_MEMORY(dest_component, src_component, byte_t, type_data->size);
}

static inline bool ct_in_table(const struct CTable * table, struct Ct ct)
{
        return map_contains(&table->ct_to_col, ct.id);
}

void move_entity(struct CTable * dest, struct CTable * src, struct Entity entity)
{
        LOG_DEBUG("Moving " ENTITY_FS " to " CTABLE_FS " from " CTABLE_FS " ...\n",
                ENTITY_FA(entity), CTABLE_FA(*dest), CTABLE_FA(*src));

        ASSERT(entity_in_table(src, entity), "No " ENTITY_FS " in " CTABLE_FS ".",
                ENTITY_FA(entity), CTABLE_FA(*src));

        ASSERT(!entity_in_table(dest, entity), "Already " ENTITY_FS " in " CTABLE_FS ".",
                ENTITY_FA(entity), CTABLE_FA(*dest));

        // Make space for a new entity in "dest" and move components from
        // "entity" in "src" to that space.
        add_entity_to_table(dest, entity);
        for (map_idx_t i = 0; i < dest->ct_to_col.length; ++i) {

                struct Ct ct;
                ct.id = dest->ct_to_col.index_to_id[i];

                if (ct_in_table(src, ct)) {
                        struct Cell src_cell = {
                                .table = src,
                                .entity = entity,
                                .ct = ct
                        };
                        struct Cell dest_cell = src_cell;
                        dest_cell.table = dest;

                        copy_cell_component(&dest_cell, &src_cell);
                }
        }

        // Remove "entity" from "src", but don't destroy the components
        // since they're now used in "dest".
        remove_table_entity(src, entity, false);
}

// Removes all entities whose IDs are in "pool" from "ctable" AND "pool".
// If "destroy" is "true", destroy their components as well.
static void remove_ctable_entity_pool(struct CTable * ctable, struct IdPool * pool, bool destroy)
{
        // While there's IDs left in "pool", take an arbitrary ID from
        // it and remove the entity.
        while (pool->len > 0) {
                struct Entity entity;
                entity.id = steal_from_id_pool(pool);
                remove_table_entity(ctable, entity, destroy);
        }
}

// Removes the entities in "ctable->removed_entities" and destroys the
// ones in "destroyed_entities".
// The function cannot be called while iterating through "ctable", as
// it may shuffle data around.
// It must be called between iterations, or some entities might be
// skipped.
static void refresh_ctable(struct CTable * ctable)
{
        // If no entities were removed or destroyed during the last
        // iteration, the table needs no refreshing.
        if (ctable->removed_entities.len == 0 && ctable->destroyed_entities.len == 0) {
                return;
        }

        // Remove "removed_entities" and "destroyed_entities" from "ctable",
        // destroying the latter.
        remove_ctable_entity_pool(ctable, &ctable->removed_entities, false);
        remove_ctable_entity_pool(ctable, &ctable->destroyed_entities, true);

        // Now that the iteration is finished (it must be; otherwise
        // calling this routine would be illegal), we make sure no
        // entities are skipped during the new one.
        // Skipping entities only happen to entities whose index
        // decreased during the same iteration, so these booleans
        // should never be true before iteration starts.
        // That was poorly explained, but then again, it's poorly
        // designed too so rather than solely blaming explainer-me,
        // blame designer-me as well.
        for (row_idx_t i = 0; i < ctable->row_count; ++i) {
                ctable->row_idx_skipped[i] = false;
        }
}

static struct Entity invalid_entity(void)
{
        struct Entity entity;
        entity.id = PCECS_INVALID_ID;
        return entity;
}

struct Entity first_entity_in_ctable(struct CTable * ctable)
{
        ASSERT(ctable->iterator.id == PCECS_INVALID_ID,
                "Cannot iterate through " CTABLE_FS " as it's already being iterated by "
                ENTITY_FS ".", CTABLE_FA(*ctable), ENTITY_FA(ctable->iterator));

        if (ctable->row_count == 0) {
                return invalid_entity();
        }

        row_idx_t row_idx = ctable->row_count - 1;
        while (ctable->row_idx_skipped[row_idx]) {

                if (row_idx == 0) {
                        return invalid_entity();
                }
                --row_idx;
        }
        ctable->iterator = ctable->row_idx_to_entity[row_idx];
        return ctable->iterator;
}

void halt_ctable_iteration(struct CTable * ctable)
{
        ctable->iterator.id = PCECS_INVALID_ID;
        refresh_ctable(ctable);
}

struct Entity next_entity_in_ctable(struct CTable * ctable, struct Entity curr_entity)
{
        ASSERT(ctable->iterator.id == curr_entity.id,
                "Iterating " CTABLE_FS " from wrong position " ENTITY_FS ".",
                CTABLE_FA(*ctable), ENTITY_FA(curr_entity));

        // No need to assert that "curr_entity" is in "ctable", because if
        // the above assertion is true and "ctable" really is being
        // iterated from "curr_entity", "curr_entity" sort of has to be in
        // the table.

        row_idx_t row_idx = ctable->entity_to_row_idx[curr_entity.id];

        // If done iterating.
        if (row_idx == 0) {
                halt_ctable_iteration(ctable);
                return invalid_entity();
        }

        row_idx_t new_row_idx = row_idx - 1;
        while (ctable->row_idx_skipped[new_row_idx]) {
                if (new_row_idx == 0) {
                        halt_ctable_iteration(ctable);
                        return invalid_entity();
                }

                --new_row_idx;
        }

        ctable->iterator = ctable->row_idx_to_entity[new_row_idx];
        return ctable->iterator;
}
