// The underlying data of entities (not all of it -- most is in "CTable"s).
// IDs of entities are mapped to this structure.
// Entities are mostly just used to mark other things (like component types
// and component table rows), so very little underlying data is needed.

#ifndef ENTITY_DATA_H
#define ENTITY_DATA_H

#include "arct.h"

#define ENTITY_DATA_FS "entity data (" ARCT_FS ")"
#define ENTITY_DATA_FA(entity_data) ARCT_FA((entity_data).arct)

struct EntityData {
        // The archetype that the entity belongs to, that is, what combination
        // of component types it contains.
        struct Arct arct;
};

// Create and initialize an "EntityData" struct.
struct EntityData create_entity_data(struct Arct arct);

// Free resources allocated by "entity_data".
void destroy_entity_data(struct EntityData * entity_data);

// Same as "destroy_entity_data", but using a void pointer parameter for
// generalized destruction function pointers.
void destroy_entity_data_void(void * entity_data);

#endif
