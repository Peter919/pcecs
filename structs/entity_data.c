#include "entity_data.h"
#include "../tools/log.h"

struct EntityData create_entity_data(struct Arct arct)
{
        LOG_DEBUG("Creating entity data from " ARCT_FS " ...\n", ARCT_FA(arct));

        struct EntityData entity_data = {
                .arct = arct
        };

        LOG_DEBUG("Created " ENTITY_DATA_FS ".\n", ENTITY_DATA_FA(entity_data));
        return entity_data;
}

void destroy_entity_data(struct EntityData * entity_data)
{
        LOG_DEBUG("Destroying " ENTITY_DATA_FS " ...\n", ENTITY_DATA_FA(*entity_data));

        // This function is only responsible for freeing the data of
        // "entity_data", not cleaning up other resources related to
        // this entity (that's "destroy_entity"'s responsibility).
        // No members of entity datas requires freeing, so this
        // function does nothing.
        (void) entity_data;
}

void destroy_entity_data_void(void * entity_data)
{
        destroy_entity_data((struct EntityData *) entity_data);
}
