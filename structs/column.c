#include "column.h"
#include "../tools/mem_tools.h"
#include "../tools/log.h"
#include "../interface/ct.h"
#include "ct_data.h"
#include "../globals/maps.h"

struct Column create_column(struct Ct ct, size_t capacity)
{
        struct Column col;

        const struct CtData * ct_data = get_map_element(&g_ct_map, ct.id);
        col.component_size = ct_data->size;

        // Allocate enough space for "capacity" components of size
        // "col.component_size".
        // "sizeof(void) == 0" isn't standard (not that I have a reason to
        // care as I only use one compiler and operating system anyway), so
        // "byte_t" is used as an allocation unit.
        col.components = ALLOC(byte_t, capacity * col.component_size);
        return col;
}

void resize_column(struct Column * col, size_t count)
{
        LOG_DEBUG("Resizing " COL_FS " to %d instances of size %d ...\n",
                COL_FA(col), (int) count, (int) col->component_size);

        // Resize "col->components" to "count" components of size
        // "col->component_size".
        // Again, sizeof(void) == 1 is not standard so "byte_t"s are
        // used instead.
        REALLOC(&col->components, byte_t, col->component_size * count);
}
