#include "../globals/id_mgrs.h"
#include <stdbool.h>
#include "../tools/log.h"
#include "../tools/debug.h"
#include "../ids/id_mgr.h"

struct IdMgr g_id_mgrs[ID_MGR_ITEM_COUNT];
bool g_id_mgrs_initialized = false;

#ifdef ASSERTIONS
static bool id_mgr_valid(enum GIdMgr id_manager)
{
        switch (id_manager) {
        case ID_MGR_ENTITIES:
        case ID_MGR_CTS:
        case ID_MGR_SYS:
        case ID_MGR_ARCTS:
                return true;
        case ID_MGR_ITEM_COUNT:
                return false;
        }
        // This is done after the switch statement rather than in a
        // default case to make sure the compiler produces a warning
        // if the switch is missing an enumeration value.
        return false;
}
#endif

void init_id_managers(void)
{
        LOG_DEBUG("Initializing global id managers ...\n");

        ASSERT(!g_id_mgrs_initialized, "Id managers already initialized.");

        // For each type of manager in "enum GIdMgr", initialize
        // corresponding "struct IdMgr".
        for (int i = 0; i < ID_MGR_ITEM_COUNT; i++) {
                g_id_mgrs[i] = create_id_manager();
        }
        g_id_mgrs_initialized = true;
}

#if defined(ASSERTIONS) || LOGGABLE(LOG_LVL_DEBUG)
        // Returns a lowercase string literal corresponding to "mgr".
        static const char * id_mgr_as_str(enum GIdMgr mgr)
        {
                switch (mgr) {
                case ID_MGR_ENTITIES:
                        return "entity";
                case ID_MGR_CTS:
                        return "component type";
                case ID_MGR_SYS:
                        return "system";
                case ID_MGR_ARCTS:
                        return "archetype";
                case ID_MGR_ITEM_COUNT:
                        break;
                }
                // Again, this is not in a default case because if it was,
                // the compiler wouldn't warn if an enumeration value has
                // no corresponding case.
                ASSERT(false, "Invalid id manager type %d.", mgr);
                return NULL;
        }
#endif

static struct IdMgr * get_id_manager(enum GIdMgr mgr)
{
        ASSERT(g_id_mgrs_initialized, "Global id managers uninitialized.");

        ASSERT(id_mgr_valid(mgr), "Invalid %s id manager enum value %d.",
                id_mgr_as_str(mgr), (int) mgr);

        return &g_id_mgrs[mgr];
}

pcecs_id_t generate_id_of_type(enum GIdMgr mgr)
{
        LOG_DEBUG("Generating id from %s id manager ...\n", id_mgr_as_str(mgr));

        struct IdMgr * id_manager_ptr = get_id_manager(mgr);
        pcecs_id_t id = generate_id(id_manager_ptr);

        LOG_DEBUG("Generated " PCECS_ID_FS ".\n", PCECS_ID_FA(id));
        return id;
}

void destroy_id_of_type(enum GIdMgr mgr, pcecs_id_t id)
{
        LOG_DEBUG("Destroying " PCECS_ID_FS " from %s id manager ...\n",
                PCECS_ID_FA(id), id_mgr_as_str(mgr));

        struct IdMgr * mgr_ptr = get_id_manager(mgr);

        ASSERT(id_in_use(mgr_ptr, id), "Invalid %s " PCECS_ID_FS ".",
                id_mgr_as_str(mgr), PCECS_ID_FA(id));

        destroy_id(mgr_ptr, id);
}
