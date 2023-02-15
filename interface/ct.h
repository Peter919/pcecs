// Components contain the data of an ECS.
// What the components themselves store is up to the user,
// but it's important to keep track of the properties of
// types of components to be able to manage them correctly
// (allocate the correct amount of space, destroying them
// properly, et cetera).
// Component types cannot be destroyed once created, but
// individual components can.

#ifndef CT_H
#define CT_H

#include <stdbool.h>
#include <stddef.h>
#include "../ids/id.h"

#define CT_FS "component type (" PCECS_ID_FS ")"
#define CT_FA(component_type) PCECS_ID_FA(component_type.id)

// "struct Ct" is simply an interface, but it
// has data belonging to it in the underlying implementation
// (see "ct_data.h").
struct Ct {
        pcecs_id_t id;
};

// Creates a new component type. "size" is the size of a
// single component, and "destructor" is the destructor
// for the component type (such as "free" for heap
// character pointers), taking a component as a void
// pointer as its parameter.
struct Ct create_ct(size_t size, void (* destructor)(void *));

// Checks if two component types are the same.
// Will return false if the arguments are referring to
// two different component types, even if their
// properties are the same.
bool cts_equal(struct Ct ct1, struct Ct ct2);

// Component types cannot be destroyed because to do so, one would
// have to destroy any archetypes containing the component type,
// and archetypes cannot be destroyed since that would require
// cleaning up a lot of data in a lot of places referencing the
// archetype.

#endif
