// Component type sets: unordered set of "struct Ct"s.

#ifndef CT_SET_H
#define CT_SET_H

#include <stdbool.h>
#include <stddef.h>
#include "../interface/ct.h"
#include "../tools/byte.h"

#define CT_SET_FS "component type set%s"
#define CT_SET_FA(ct_set) ""

struct CtSet {
        size_t size;
        byte_t * contents;
};

// Creates a new component type set with no component types.
struct CtSet create_ct_set(void);

// Destroys a component type set; does nothing to the
// actual component types.
void destroy_ct_set(struct CtSet * set);

// Copies the component types of one set to another.
// Not guaranteed to work if "dest" isn't initialized
// before-hand.
void copy_ct_set(struct CtSet * dest, const struct CtSet * src);

// Creates a new set that contains all component types
// that are either in "set1" or "set2".
struct CtSet ct_set_union(const struct CtSet * set1, const struct CtSet * set2);

// Returns "true" if "set" contains "ct", "false" otherwise.
bool ct_in_set(const struct CtSet * set, struct Ct ct);

// Returns "true" if "subset" is a subset of "superset" (all
// component types in "subset" are also in "superset"),
// "false" otherwise.
// "superset" does not have to be a proper superset, so the
// function returns "true" if the sets are the same.
// Passing two pointers to the same set is valid.
bool ct_set_in_set(const struct CtSet * subset, const struct CtSet * superset);

// Returns whether or not two sets contain the exact same
// component types.
// Passing two pointers to the same set is valid.
bool ct_sets_equal(const struct CtSet * set1, const struct CtSet * set2);

// Returns "true" iff "set" contains no components.
bool ct_set_empty(const struct CtSet * set);

// The number of component types in "set".
size_t cts_in_set_count(const struct CtSet * set);

// Adds "ct" to "set", which cannot already contain "ct".
void add_ct_to_set(struct CtSet * set, struct Ct ct);

// Add the "count" first "Ct"s of "list" to "set".
void add_ct_list_to_set(struct CtSet * set, struct Ct * list, size_t count);

// Removes "ct" from "set", which must contain "ct" when
// calling the function.
void remove_ct_from_set(struct CtSet * set, struct Ct ct);

// Finds an arbitrary "first" component type that "set" contains.
// Use in combination with "next_ct_in_set" to iterate through
// the component types of a set.
struct Ct first_ct_in_set(const struct CtSet * set);

// Finds an arbitrary "next" component type that "set" contains,
// after "start_ct".
// Returns a component type with ID = "PCECS_INVALID_ID" if
// "start_ct" is the "last" component type.
// Removing components while iterating through a set is legal,
// and the removed component(s) won't be iterated through.
// Adding components while iterating through a set is not legal.
struct Ct next_ct_in_set(const struct CtSet * set, struct Ct start_ct);

#endif
