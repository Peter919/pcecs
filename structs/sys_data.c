#include "sys_data.h"
#include "../tools/log.h"

static struct SysFuncs create_sys_funcs(void)
{
        return (struct SysFuncs) {
                .start = NULL,
                .update = NULL,
                .draw = NULL,
                .destroy = NULL
        };
}

struct SysData create_sys_data(const struct CtSet * requirements)
{
        LOG_DEBUG("Creating system info ...\n");

        struct SysData sys_data;
        // "requirements" (the argument) is deep-copied so the system
        // doesn't unexpectedly change when that variable changes.
        sys_data.requirements = create_ct_set();
        copy_ct_set(&sys_data.requirements, requirements);
        sys_data.funcs = create_sys_funcs();

        LOG_DEBUG("Created " SYS_DATA_FS ".\n", SYS_DATA_FA(sys_data));
        return sys_data;
}

void destroy_sys_data(struct SysData * sys_data)
{
        (void) sys_data;
        // TODO: code
}

void destroy_sys_data_void(void * sys_data)
{
        destroy_sys_data((struct SysData *) sys_data);
}
