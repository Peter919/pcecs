#include "cgroup.h"
#include <stdbool.h>
#include "../tools/log.h"
#include "../structs/arct.h"
#include "../globals/maps.h"
#include "../structs/sys_data.h"

static bool ct_in_sys(struct Sys sys, struct Ct ct)
{
        const struct SysData * sys_data = get_map_element(&g_sys_map, sys.id);
        return ct_in_set(&sys_data->requirements, ct);
}

void * get_component(struct CGroup * cgroup, struct Ct ct)
{
        ASSERT_OR_HANDLE(ct_in_sys(cgroup->sys, ct), NULL, "No " CT_FS " in " SYS_FS ".",
                CT_FA(ct), SYS_FA(cgroup->sys));

        return get_component_from_entity(cgroup->entity, ct);
}
