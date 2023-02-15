#include "id_mgr.h"
#include <stdbool.h>
#include "../tools/mem_tools.h"
#include "../tools/debug.h"
#include "../tools/log.h"

struct IdMgr create_id_manager(void)
{
        LOG_DEBUG("Creating id manager ...\n");

        struct IdMgr mgr = {
                .max_id = 0,
                .unused_ids = create_id_pool()
        };
        return mgr;
}

void destroy_id_manager(struct IdMgr * mgr)
{
        LOG_DEBUG("Destroying id manager ...\n");
        destroy_id_pool(&mgr->unused_ids);
}

pcecs_id_t ids_in_use_count(const struct IdMgr * mgr)
{
        // Since PCECS_INVALID_ID equals 0, the generating of
        // IDs start at 1. Therefore, the nth id will equal n,
        // and, assuming all IDs from 1 to mgr->max_id are in
        // use, max_id will equal the total number of IDs.
        // Not all IDs are guaranteed to be used, however, so
        // the number of unused IDs must be subtracted from
        // the result.
        STATIC_ASSERT(PCECS_INVALID_ID == 0);
        return mgr->max_id - (pcecs_id_t) mgr->unused_ids.len;
}

bool id_in_use(const struct IdMgr * mgr, pcecs_id_t id)
{
        if (id > mgr->max_id) {
                return false;
        }
        if (id_pool_contains(&mgr->unused_ids, id)) {
                return false;
        }
        return true;
}

pcecs_id_t generate_id(struct IdMgr * mgr)
{
        LOG_DEBUG("Generating id ...\n");

        if (mgr->unused_ids.len > 0) {
                return steal_from_id_pool(&mgr->unused_ids);
        }

        // PCECS_INVALID_ID, which equals 0, is definitely
        // avoided here since we pre-increment max_id.
        // Since max_id is initialized to 0, the first ID
        // generated will be 1.
        STATIC_ASSERT(PCECS_INVALID_ID == 0);
        pcecs_id_t id = ++mgr->max_id;

        LOG_DEBUG("Generated " PCECS_ID_FS ".\n", PCECS_ID_FA(id));
        return id;
}

void destroy_id(struct IdMgr * mgr, pcecs_id_t id)
{
        LOG_DEBUG("Destroying " PCECS_ID_FS " ...\n", PCECS_ID_FA(id));
        ASSERT(id_in_use(mgr, id), "Id not in use.");

        add_to_id_pool(&mgr->unused_ids, id);
}
