// The underlying data of archetypes.
// IDs of archetypes are mapped to this structure.

#ifndef ARCT_DATA_H
#define ARCT_DATA_H

#include "../interface/ct_set.h"
#include "ctable.h"
#include "arct_edges.h"

#define ARCT_DATA_FS "archetype data%s"
#define ARCT_DATA_FA(arct_data) ""

struct ArctData {
        // The set of component types that all entities belonging
        // to this archetype contains.
        struct CtSet ct_set;
        // The entities of this archetype and their components, in
        // one giant table with quite fast access (not my idea of
        // course).
        struct CTable ctable;
        // A set of archetypes with the exact same component types
        // as this one, except there's one missing or one extra.
        // Used to speed up searching for similar archetypes when
        // adding or removing a component from an entity.
        struct ArctEdges edges;
        // Systems affecting this archetype.
        // We could go the other way around and store a pool of
        // archetype IDs in "SysData"s, but then we'd jump a lot
        // from archetype to archetype, and jumping around
        // between different unrelated memory addresses is slow.
        struct IdPool systems;
};

// Creates and initializes underlying data for "arct"
// where its entities have the component types in
// "ct_set".
struct ArctData create_arct_data(struct Arct arct, const struct CtSet * ct_set);

#endif
