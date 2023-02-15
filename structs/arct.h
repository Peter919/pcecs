// Entities with the exact same components are grouped together in archetypes.

#ifndef ARCT_H
#define ARCT_H

#include "../ids/id.h"
#include "../interface/ct_set.h"
#include "../interface/sys_funcs.h"

#define ARCT_FS "archetype (" PCECS_ID_FS ")"
#define ARCT_FA(arct) PCECS_ID_FA((arct).id)

struct Arct {
        pcecs_id_t id;
};

// Creates an archetype with the components specified in "ct_set".
// If an archetype with those components already exists, that
// archetype is returned.
struct Arct create_arct(const struct CtSet * ct_set);

// Check for equality between two archetypes (whether they're the
// same object; not whether they have the same component types
// -- no different archetypes should have the same set of components
// since "create_arct" will attempt to find an archetype matching
// the component type set inputted).
bool arcts_equal(struct Arct arct1, struct Arct arct2);

// Execute one function depending on 'func_type' for each system in
// 'arct'.
void exec_arct_systems(struct Arct arct, enum SysFuncType func_type);

// Archetypes cannot be destroyed, since they're referenced a lot of
// places and cleaning that up would take a lot of time.

#endif
