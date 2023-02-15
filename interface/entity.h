// Conceptually, entities are groups of components that belong together.
// In the implementation, however, those components are stored in the
// component tables (see "struct CTable") in archetypes. All information
// needed about entities themselves is their ID (to access the correct
// entity in those tables) and which component table they belong to.
// Their ID is here, and the rest of the data is in "struct EntityData".

#ifndef ENTITY_H
#define ENTITY_H

#include <stdbool.h>
#include "ct.h"
#include "../ids/id.h"

#define ENTITY_FS "entity (" PCECS_ID_FS ")"
#define ENTITY_FA(entity) PCECS_ID_FA((entity).id)

struct Entity {
        pcecs_id_t id;
};

// Create an entity and initialize all of its underlying data (not just
// the structure itself).
struct Entity create_entity(void);

// Destroy an entity and all of its underlying data (not just the struct).
void destroy_entity(struct Entity * entity);

// Check if "entity1" and "entity2" are the same object.
bool entities_equal(struct Entity entity1, struct Entity entity2);

// Check if "entity" contains "ct".
bool contains_component(struct Entity entity, struct Ct ct);

// Add "ct" to "entity". The new component will contain junk data, but can
// be initialized using "get_component" or "get_component_from_entity".
void add_component(struct Entity entity, struct Ct ct);

// Remove "ct" from "entity".
void remove_component(struct Entity entity, struct Ct ct);

// Returns the component of type "ct" in "entity".
// Cannot be called if "entity" doesn't contain "ct".
// "get_component" is recommended over this one, as it's safer.
void * get_component_from_entity(struct Entity entity, struct Ct ct);

#endif
