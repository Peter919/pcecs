#include "ct_data.h"
#include <stdbool.h>

#define ARCT_POOL_CAPACITY_MUL 2

struct CtData create_ct_data(size_t size, void (* destructor)(void * component))
{
        LOG_DEBUG("Creating component type info of size %d ...\n", (int) size);

        struct CtData ct_data = {
                // A newly created component type doesn't belong to any archetypes.
                .arcts = create_id_pool(),
                .size = size,
                .destructor = destructor
        };

        LOG_DEBUG("Created " CT_DATA_FS ".\n", CT_DATA_FA(ct_data));

        return ct_data;
}

void add_arct_to_ct(struct CtData * ct, struct Arct arct)
{
        add_to_id_pool(&ct->arcts, arct.id);
}
