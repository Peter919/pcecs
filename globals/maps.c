#include "maps.h"
#include <stdbool.h>
#include "../structs/map.h"

#include "../structs/entity_data.h"
#include "../structs/ct_data.h"
#include "../structs/sys_data.h"
#include "../structs/arct_data.h"

void init_maps(void)
{
        static bool s_maps_initialized = false;
        // May be unused if "DEBUG_OFF".
        (void) s_maps_initialized;

        ASSERT(!s_maps_initialized, "Maps already initialized.");

        // "CtData"s and "ArctData"s cannot be destroyed, so destructors
        // are simply not provided.
        g_entity_map = create_map(sizeof(struct EntityData), destroy_entity_data_void);
        g_ct_map = create_map(sizeof(struct CtData), NULL);
        g_sys_map = create_map(sizeof(struct SysData), destroy_sys_data_void);
        g_arct_map = create_map(sizeof(struct ArctData), NULL);

        s_maps_initialized = true;
}
