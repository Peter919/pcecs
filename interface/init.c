#include <stdbool.h>
#include "../tools/log.h"
#include "../globals/id_mgrs.h"
#include "../globals/maps.h"

void init_pcecs(void)
{
        static bool s_initialized = false;
        // "s_initialized" unused if assertions are disabled; this line
        // prevents a warning.
        (void) s_initialized;

        LOG_DEBUG("Initializing pcecs ...\n");
        ASSERT_OR_HANDLE(!s_initialized, , "Pcecs already initialized.");

        init_id_managers();
        init_maps();
        s_initialized = true;

        LOG_INFO("Pcecs initialized.\n");
        LOG_DEBUG_HIDE_LEVEL("\n");
}
