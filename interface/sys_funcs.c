#include "sys_funcs.h"
#include "../globals/maps.h"
#include "../structs/sys_data.h"
#include "../tools/debug.h"

static sys_func_t * get_sys_func_ptr(struct Sys sys, enum SysFuncType type)
{
        struct SysData * sys_data = get_map_element(&g_sys_map, sys.id);

        switch (type) {
        case SYS_START:
                return &sys_data->funcs.start;
        case SYS_UPDATE:
                return &sys_data->funcs.update;
        case SYS_DRAW:
                return &sys_data->funcs.draw;
        case SYS_DESTROY:
                return &sys_data->funcs.destroy;
        }
        ASSERT(false, "Invalid system function %d.", (int) type);
        return NULL;
}

static bool valid_sys_func_type(enum SysFuncType type)
{
        switch (type) {
        case SYS_START:
        case SYS_UPDATE:
        case SYS_DRAW:
        case SYS_DESTROY:
                return true;
        }
        return false;
}

sys_func_t get_sys_func(struct Sys sys, enum SysFuncType type)
{
        ASSERT_OR_HANDLE(map_contains(&g_sys_map, sys.id), NULL,
                "Non-existent " SYS_FS ".", SYS_FA(sys));

        ASSERT_OR_HANDLE(valid_sys_func_type(type), NULL,
                "Invalid system function type %d.", (int) type);

        return *get_sys_func_ptr(sys, type);
}

void set_sys_func(struct Sys sys, enum SysFuncType func_type, sys_func_t func)
{
        ASSERT_OR_HANDLE(map_contains(&g_sys_map, sys.id), ,
                "Non-existent " SYS_FS ".", SYS_FA(sys));

        ASSERT_OR_HANDLE(valid_sys_func_type(func_type), ,
                "Invalid system function type %d.", (int) func_type);

        sys_func_t * old_func = get_sys_func_ptr(sys, func_type);
        *old_func = func;
}
