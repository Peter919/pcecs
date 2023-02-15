// An interface that makes it possible to generate IDs of
// specific types (entity, archetype, etc.) without needing
// global ID managers exposed to the entire project.

#ifndef ID_MGRS_H
#define ID_MGRS_H

#include "../ids/id.h"

enum GIdMgr {
        ID_MGR_ENTITIES,
        ID_MGR_CTS,
        ID_MGR_SYS,
        ID_MGR_ARCTS,
        ID_MGR_ITEM_COUNT
};

void init_id_managers(void);

// Generates IDs for a specific purpose (see "enum GIdMgr").
// IDs generated for the same purpose are always unique (although
// destroyed IDs might be used later; see "destroy_id_of_type")
// but IDs used for different purposes might be the same since
// they aren't supposed to be compared anyway.
pcecs_id_t generate_id_of_type(enum GIdMgr mgr);

// Marks an ID for later use.
void destroy_id_of_type(enum GIdMgr mgr, pcecs_id_t id);

#endif
