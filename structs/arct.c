#include "arct.h"
#include "../interface/ct_set.h"
#include "../globals/id_mgrs.h"
#include "arct_data.h"
#include "../globals/maps.h"
#include "../tools/log.h"
#include "ct_data.h"
#include "sys_data.h"
#include "../interface/sys_funcs.h"

// Returns an archetype with no component types if it exists,
// or an archetype with id == "PCECS_INVALID_ID" if not.
static struct Arct find_empty_arct(void)
{
        struct CtSet empty_ct_set = create_ct_set();

        const struct ArctData * arct_datas = g_arct_map.values;

        for (size_t i = 0; i < g_arct_map.length; ++i) {
                if (ct_sets_equal(&arct_datas[i].ct_set, &empty_ct_set)) {

                        pcecs_id_t arct_id = g_arct_map.index_to_id[i];

                        destroy_ct_set(&empty_ct_set);
                        return (struct Arct) {
                                .id = arct_id
                        };
                }
        }

        destroy_ct_set(&empty_ct_set);
        return (struct Arct) {
                .id = PCECS_INVALID_ID
        };
}

// Returns an archetype with ID PCECS_INVALID_ID if the archetype isn't found.
static struct Arct find_arct(const struct CtSet * ct_set)
{
        // We're trying to narrow our search down to archetypes containing the
        // first component type in "ct_set". If there is no first component
        // type in "ct_set", however, we simply try to find an empty "CtSet".
        struct Ct ct = first_ct_in_set(ct_set);
        if (ct.id == PCECS_INVALID_ID) {
                return find_empty_arct();
        }

        const struct CtData * ct_data = get_map_element(&g_ct_map, ct.id);

        // For each archetype containing the first component type in
        // "ct_set" ...
        for (size_t i = 0; i < ct_data->arcts.len; ++i) {
                struct Arct arct;
                arct.id = ct_data->arcts.contents[i];

                const struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);

                // Return the archetype if its "CtSet" matches "ct_set".
                if (ct_sets_equal(&arct_data->ct_set, ct_set)) {
                        return arct;
                }
        }

        return (struct Arct) {
                .id = PCECS_INVALID_ID
        };
}

// If an archetype of the components in "ct_set" already exists,
// it's returned. Otherwise, a new one is created.
struct Arct create_arct(const struct CtSet * ct_set)
{
        // Try to find an archetype and return if it's found.
        struct Arct found_arct = find_arct(ct_set);
        if (found_arct.id != PCECS_INVALID_ID) {
                return found_arct;
        }

        LOG_DEBUG("Creating archetype from " CT_SET_FS ".\n",
                CT_SET_FA(*ct_set));

        struct Arct new_arct = {
                .id = generate_id_of_type(ID_MGR_ARCTS)
        };

        struct ArctData arct_data;
        arct_data = create_arct_data(new_arct, ct_set);
        add_to_map(&g_arct_map, new_arct.id, &arct_data);

        // Iterate through components in the created archetype.
        // For each component, add the archetype to its list of archetypes.
        struct Ct ct = first_ct_in_set(ct_set);
        while (ct.id != PCECS_INVALID_ID) {

                struct CtData * ct_data;
                ct_data = get_map_element(&g_ct_map, ct.id);
                add_arct_to_ct(ct_data, new_arct);

                ct = next_ct_in_set(ct_set, ct);
        }

        LOG_DEBUG("Created " ARCT_FS ".\n\n", ARCT_FA(new_arct));
        return new_arct;
}

bool arcts_equal(struct Arct arct1, struct Arct arct2)
{
        return arct1.id == arct2.id;
}

static void exec_single_system(struct Arct arct, struct Sys sys, enum SysFuncType func_type)
{
        sys_func_t sys_func = *get_sys_func(sys, func_type);
        if (sys_func == NULL) {
                return;
        }

        struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);

        struct CGroup cgroup;
        cgroup.sys = sys;

        struct Entity entity = first_entity_in_ctable(&arct_data->ctable);
        while (entity.id != PCECS_INVALID_ID) {

                cgroup.entity = entity;
                sys_func(cgroup);

                entity = next_entity_in_ctable(&arct_data->ctable, entity);
        }
}

void exec_arct_systems(struct Arct arct, enum SysFuncType func_type)
{
        const struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);
        for (size_t i = 0; i < arct_data->systems.len; ++i) {

                struct Sys sys;
                sys.id = arct_data->systems.contents[i];

                exec_single_system(arct, sys, func_type);
        }
}
