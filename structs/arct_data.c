#include "arct_data.h"
#include "../interface/sys.h"
#include "../globals/maps.h"
#include "sys_data.h"
#include "ct_data.h"

static void add_systems_to_arct_data(struct ArctData * arct_data)
{
        const struct SysData * sys_datas = g_sys_map.values;

        for (map_idx_t i = 0; i < g_sys_map.length; ++i) {

                if (ct_set_in_set(&sys_datas[i].requirements, &arct_data->ct_set)) {
                        add_to_id_pool(&arct_data->systems, g_sys_map.index_to_id[i]);
                }
        }
}

struct ArctData create_arct_data(struct Arct arct, const struct CtSet * ct_set)
{
        LOG_DEBUG("Creating archetype info from " CT_SET_FS " ...\n",
                CT_SET_FA(ct_set));

        // Create a copy of "ct_set" to make sure that if
        // "ct_set" (the parameter) changes, this "struct
        // ArctData"'s "ct_set" won't.
        struct CtSet ct_set_cpy = create_ct_set();
        copy_ct_set(&ct_set_cpy, ct_set);

        struct ArctData arct_data;
        arct_data.ct_set = ct_set_cpy;

        // Create a component table with the component types
        // of "arct_data", but no entities.
        arct_data.ctable = create_ctable(&arct_data.ct_set);

        arct_data.edges = create_arct_edges(arct);

        arct_data.systems = create_id_pool();
        add_systems_to_arct_data(&arct_data);

        return arct_data;
}
