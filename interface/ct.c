#include "ct.h"
#include "../structs/ct_data.h"
#include "../globals/id_mgrs.h"
#include "../globals/maps.h"
#include "../tools/log.h"
#include "../tools/mem_tools.h"

void noop(void * arg)
{
        (void) arg;
}

struct Ct create_ct(size_t size, void (* destructor)(void *))
{
        LOG_DEBUG("Creating component type ...\n");

        struct Ct ct = {
                .id = generate_id_of_type(ID_MGR_CTS)
        };

        // Create underlying data for the new component type, and
        // add it to the global component type map.
        // If no destructor is provided, simply don't destroy by
        // passing a no-op as the "destructor" argument.
        struct CtData data = create_ct_data(size, destructor ? destructor : noop);
        add_to_map(&g_ct_map, ct.id, &data);

        LOG_INFO("Created " CT_FS ".\n", CT_FA(ct));
        LOG_DEBUG_HIDE_LEVEL("\n");
        return ct;
}

bool cts_equal(struct Ct ct1, struct Ct ct2)
{
        return ct1.id == ct2.id;
}
