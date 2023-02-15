#include "sys.h"
#include "../globals/id_mgrs.h"
#include "../tools/log.h"
#include "../structs/sys_data.h"
#include "../globals/maps.h"
#include "../structs/ct_data.h"
#include "../structs/arct_data.h"

static void call_start_on_arct(struct Sys sys, struct Arct arct)
{
        struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);
        ASSERT_OR_HANDLE(!ctable_being_iterated(&arct_data->ctable), ,
                "Entities already being iterated through. "
                "Is a system being created while entities are updating?");

        sys_func_t start_func = get_sys_func(sys, SYS_START);
        struct CGroup cgroup;
        cgroup.sys = sys;

        struct Entity entity = first_entity_in_ctable(&arct_data->ctable);
        while (entity.id != PCECS_INVALID_ID) {

                cgroup.entity = entity;
                if (start_func) {
                        start_func(cgroup);
                }

                entity = next_entity_in_ctable(&arct_data->ctable, entity);
        }
}

static void add_sys_to_arcts(struct Sys sys)
{
        const struct SysData * sys_data = get_map_element(&g_sys_map, sys.id);

        // Completely arbitrary component type, simply used to narrow
        // the search for archetypes down.
        struct Ct ct = first_ct_in_set(&sys_data->requirements);
        const struct CtData * ct_data = get_map_element(&g_ct_map, ct.id);

        // Archetypes only match "sys" if they include all of its
        // required component types.
        // This means that we can speed up the search for archetypes
        // by only iterating through archetypes containing at least
        // one specific but arbitrary required component.
        for (size_t i = 0; i < ct_data->arcts.len; ++i) {
                struct Arct arct;
                arct.id = ct_data->arcts.contents[i];
                struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);

                if (ct_set_in_set(&sys_data->requirements, &arct_data->ct_set)) {
                        add_to_id_pool(&arct_data->systems, sys.id);
                        call_start_on_arct(sys, arct);
                }
        }
}

struct Sys create_sys(const struct CtSet * requirements, sys_func_t start_func)
{
        LOG_DEBUG("Creating system ...\n");

        ASSERT_OR_HANDLE(!ct_set_empty(requirements), (struct Sys) {.id = PCECS_INVALID_ID},
                "Cannot create system with no requirements.");

        struct Sys sys = {
                .id = generate_id_of_type(ID_MGR_SYS)
        };

        // Create underlying data for the system and add it
        // to the global system map. Duh-doy!
        struct SysData sys_data = create_sys_data(requirements);
        add_to_map(&g_sys_map, sys.id, &sys_data);

        set_sys_func(sys, SYS_START, start_func);

        add_sys_to_arcts(sys);

        LOG_INFO("Created " SYS_FS ".\n", SYS_FA(sys));
        LOG_DEBUG_HIDE_LEVEL("\n");
        return sys;
}

static void remove_sys_from_arcts(struct Sys sys)
{
        struct SysData * sys_data = get_map_element(&g_sys_map, sys.id);

        struct Ct ct = first_ct_in_set(&sys_data->requirements);
        const struct CtData * ct_data = get_map_element(&g_ct_map, ct.id);

        for (size_t i = 0; i < ct_data->arcts.len; ++i) {
                struct Arct arct;
                arct.id = ct_data->arcts.contents[i];
                struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);

                if (id_pool_contains(&arct_data->systems, sys.id)) {
                        remove_from_id_pool(&arct_data->systems, sys.id);
                }
        }
}

void destroy_sys(struct Sys * sys)
{
        ASSERT_OR_HANDLE(map_contains(&g_sys_map, sys->id), ,
                "Cannot destroy non-existent " SYS_FS ".", SYS_FA(*sys));

        remove_sys_from_arcts(*sys);

        // TODO: code
        destroy_id_of_type(ID_MGR_SYS, sys->id);
}

bool sys_equal(struct Sys sys1, struct Sys sys2)
{
        return sys1.id == sys2.id;
}

static void exec_all_systems(enum SysFuncType func_type)
{
        // ENDELIG!
        for (map_idx_t i = 0; i < g_arct_map.length; ++i) {
                struct Arct arct;
                arct.id = g_arct_map.index_to_id[i];

                exec_arct_systems(arct, func_type);
        }
}

void update_entities(void)
{
        exec_all_systems(SYS_UPDATE);
}

void draw_entities(void)
{
        exec_all_systems(SYS_DRAW);
}
