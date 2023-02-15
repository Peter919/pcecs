// The underlying data of systems.
// IDs of systems are mapped to this structure.

#ifndef SYS_DATA_H
#define SYS_DATA_H

#include "../interface/ct_set.h"
#include "../interface/cgroup.h"
#include "../interface/sys_funcs.h"

#define SYS_DATA_FS "system data%s"
#define SYS_DATA_FA(sys_data) ""

// Functions affecting the entities that match the requirements of
// a system.
struct SysFuncs {
        sys_func_t start;
        sys_func_t update;
        sys_func_t draw;
        sys_func_t destroy;
};

struct SysData {
        struct CtSet requirements;
        struct SysFuncs funcs;
};

// Create and initialize a "SysData" structure.
struct SysData create_sys_data(const struct CtSet * requirements);

// Free the resources allocated by "sys_data".
void destroy_sys_data(struct SysData * sys_data);

// Same as "destroy_sys_data", but using a void pointer argument
// to generalize.
void destroy_sys_data_void(void * sys_data);

#endif
