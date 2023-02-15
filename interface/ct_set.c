#include "ct_set.h"
#include <stdbool.h>
#include "../interface/ct.h"
#include "../tools/debug.h"
#include "../tools/byte.h"
#include "../globals/maps.h"
#include "../tools/mem_tools.h"

#define BITS_IN_BYTE CHAR_BIT

struct CtSet create_ct_set(void)
{
        struct CtSet ct_set;
        ct_set.size = 0;
        ct_set.contents = ALLOC(byte_t, ct_set.size);

        LOG_DEBUG("Created " CT_SET_FS ".\n",
                CT_SET_FA(ct_set));
        return ct_set;
}

void destroy_ct_set(struct CtSet * ct_set)
{
        FREE(ct_set->contents);
}

void copy_ct_set(struct CtSet * dest, const struct CtSet * src)
{
        LOG_DEBUG("Copying " CT_SET_FS ".\n", CT_SET_FA(src));

        FREE(dest->contents);
        *dest = *src;
        dest->contents = ALLOC(byte_t, dest->size);
        COPY_MEMORY(dest->contents, src->contents, byte_t, dest->size);
}

static void resize_ct_set(struct CtSet * set, size_t size)
{
        LOG_DEBUG("Resizing " CT_SET_FS " from size %d to %d.\n",
                CT_SET_FA(set), (int) set->size, (int) size);

        REALLOC(&set->contents, byte_t, size);
        size_t prev_size = set->size;
        set->size = size;

        // If any new bytes were allocated
        if (set->size > prev_size) {
                // All  new bits set to 0 since component type sets
                // include no components beyond their size, so when
                // it's resized none of the new component types it
                // has space for are actually in it.
                // The newly allocated bytes start on the element
                // after what was previously the last element
                // (&set->contents[(prev_size - 1) + 1])
                // The amount of newly allocated bytes equals the
                // previous length subtracted from the new length.
                byte_t * first_new_byte = &set->contents[prev_size];
                size_t new_bytes_count = set->size - prev_size;
                SET_MEMORY(first_new_byte, 0, byte_t, new_bytes_count);
        }
}

struct CtSet ct_set_union(const struct CtSet * set1, const struct CtSet * set2)
{
        LOG_DEBUG("Finding the union of " CT_SET_FS " and " CT_SET_FS ".\n",
                CT_SET_FA(set1), CT_SET_FA(set2));

        // If the sets have the same size, it doesn't matter which
        // set is assigned to which of these two variables.
        const struct CtSet * largest_set = set1->size > set2->size ? set1 : set2;
        const struct CtSet * smallest_set = largest_set == set1 ? set2 : set1;

        // Make the new set equal the largest one of the sets, and
        // add the components from the smaller set by bitwise-or-ing
        // it with that one. The smaller set is guaranteed to have a
        // size smaller or equal to the longer one, so "smallest_set->
        // size" is not out of bounds.
        struct CtSet new_set = create_ct_set();
        copy_ct_set(&new_set, largest_set);

        for (size_t i = 0; i < smallest_set->size; ++i) {
                new_set.contents[i] |= smallest_set->contents[i];
        }
        return new_set;
}

static void remove_trailing_null_bytes(struct CtSet * set)
{
        LOG_DEBUG("Removing trailing null bytes from " CT_SET_FS ".\n",
                CT_SET_FA(*set));

        size_t bytes_to_remove = 0;
        while (set->contents[set->size - bytes_to_remove - 1] == 0x0) {
                ++bytes_to_remove;
        }
        if (bytes_to_remove != 0) {
                resize_ct_set(set, set->size - bytes_to_remove);
        }
}

static size_t ct_set_bit_count(const struct CtSet * set)
{
        return set->size * BITS_IN_BYTE;
}

static size_t idx_of_byte(struct Ct ct)
{
        return (size_t) ct.id / BITS_IN_BYTE;
}

// LSB is index 0, MSB is index 7.
// That might seem strange since the bits in a component type
// set will end up reading backwards compared to the bytes,
// but it's reasonable to work with since bit indices and byte
// indices are split into different routines, and
// "1 << idx_of_bit_within_byte(ct)"
// is much easier to work with than
// "(1 << (BITS_IN_BYTE - 1)) >> idx_of_bit_within_byte(ct)".
static int idx_of_bit_within_byte(struct Ct ct)
{
        return (int) ct.id % BITS_IN_BYTE;
}

bool ct_in_set(const struct CtSet * set, struct Ct ct)
{
        if ((size_t) ct.id > ct_set_bit_count(set)) {
                return false;
        }
        byte_t ct_byte = set->contents[idx_of_byte(ct)];
        byte_t ct_bit = (ct_byte >> idx_of_bit_within_byte(ct)) & 1;
        return ct_bit ? true : false;
}

bool ct_set_in_set(const struct CtSet * subset, const struct CtSet * superset)
{
        // CtSet-s are guaranteed to have no trailing null bytes,
        // so if one set is longer than the other, it's guaranteed
        // to have a 1 bit than the other doesn't have and is
        // therefore not a subset.
        if (subset->size > superset->size) {
                return false;
        }

        for (size_t i = 0; i < subset->size; ++i) {
                // Finds all the bits subset[i] has and superset[i] hasn't.
                // None of those bits are supposed to exist, of course, so
                // if the result isn't a null byte, "subset" isn't a subset.
                if (subset->contents[i] & ~superset->contents[i]) {
                        return false;
                }
        }
        return true;
}

bool ct_sets_equal(const struct CtSet * set1, const struct CtSet * set2)
{
        // There are no trailing null bytes in sets, so equal
        // sets have equal length
        if (set1->size != set2->size) {
                return false;
        }

        return MEMORY_EQUALS(set1->contents, set2->contents, byte_t, set1->size);
}

bool ct_set_empty(const struct CtSet * set)
{
        // Sets are no larger than they need to be, so any non-empty
        // set has a size of 0.
        return set->size == 0;
}

// All are aware brainy Brian brought counting computer code.
static size_t kernighan_bit_count(byte_t byte)
{
        byte_t bits_left = byte;
        size_t count = 0;
        while (bits_left)
        {
                // The subtraction will remove the least significant 1 bit
                // by carrying it to all the 0 bits before it. By bitwise
                // and-ing this value by the actual number, we remove that
                // 1 bit and keep the 0s behind it off.
                bits_left &= bits_left - 1;
                ++count;
        }

        return count;
}

size_t cts_in_set_count(const struct CtSet * set)
{
        // Each bit in the set represents a component type, so
        // the total number of component types in the set equals
        // the total number of 1 bits.
        size_t count = 0;
        for (size_t i = 0; i < set->size; ++i) {
                count += kernighan_bit_count(set->contents[i]);
        }
        return count;
}

void add_ct_to_set(struct CtSet * set, struct Ct ct)
{
        LOG_DEBUG("Adding " CT_FS " to " CT_SET_FS ".\n",
                CT_FA(ct), CT_SET_FA(*set));

        ASSERT_OR_HANDLE(map_contains(&g_ct_map, ct.id), ,
                "Cannot add non-existent " CT_FS " to " CT_SET_FS ".", CT_FA(ct), CT_SET_FA(*set));

        ASSERT_OR_HANDLE(!ct_in_set(set, ct), , "Ct already in set.");

        if ((size_t) ct.id >= ct_set_bit_count(set)) {
                // The component type set must be large enough to
                // reference the byte at the index of "ct". Since
                // arrays are 0-indexed, the size must be one element
                // longer than the index of the last element.
                resize_ct_set(set, (size_t) idx_of_byte(ct) + 1);
        }

        byte_t * ct_byte = &set->contents[idx_of_byte(ct)];
        *ct_byte |= 1 << idx_of_bit_within_byte(ct);
}

void add_ct_list_to_set(struct CtSet * set, struct Ct * list, size_t count)
{
        for (size_t i = 0; i < count; ++i) {
                add_ct_to_set(set, list[i]);
        }
}

void remove_ct_from_set(struct CtSet * set, struct Ct ct)
{
        LOG_DEBUG("Removing " CT_FS " from " CT_SET_FS " ...\n",
                CT_FA(ct), CT_SET_FA(*set));

        ASSERT_OR_HANDLE(map_contains(&g_ct_map, ct.id), , "Cannot remove non-existent " CT_FS
                " from " CT_SET_FS ".", CT_FA(ct), CT_SET_FA(*set));

        ASSERT_OR_HANDLE(ct_in_set(set, ct), , "No " CT_FS " in " CT_SET_FS ".",
                CT_FA(ct), CT_SET_FA(*set));

        byte_t * ct_byte = &set->contents[idx_of_byte(ct)];
        *ct_byte &= ~(1 << idx_of_bit_within_byte(ct));

        remove_trailing_null_bytes(set);
}

struct Ct first_ct_in_set(const struct CtSet * set)
{
        // Start at ID 0 and increment it until a component
        // type with that ID is found in "set".
        // Stops iterating if the end of the set is reached.
        for (size_t i = 0; i < set->size * BITS_IN_BYTE; ++i) {
                struct Ct ct = {
                        .id = i
                };

                if (ct_in_set(set, ct)) {
                        return ct;
                }
        }

        // End of set reached; no component type found.
        // This should really only happen if the set has
        // size 0 since "ct_set"s are guaranteed to be no
        // larger than they absolutely need to be, and any
        // size is too large for no component types.
        ASSERT(set->size == 0, "Non-zero size (%d) of " CT_SET_FS ", but no component types.",
                CT_SET_FA(*set));

        return (struct Ct) {
                .id = PCECS_INVALID_ID
        };
}

struct Ct next_ct_in_set(const struct CtSet * set, struct Ct start_ct)
{
        // From the component type right after "start_ct"
        // until the end of the component type set, loop
        // through each possible ID and return a component
        // type with that ID if it's in the set.
        size_t ct_id = (size_t) start_ct.id + 1;
        while (ct_id < set->size * BITS_IN_BYTE) {
                struct Ct ct = {
                        .id = ct_id
                };

                if (ct_in_set(set, ct)) {
                        return ct;
                }
                ++ct_id;
        }

        // Next type not found; return component type with
        // invalid (never generated by "IdMgr"s ID.
        return (struct Ct) {
                .id = PCECS_INVALID_ID
        };
}
