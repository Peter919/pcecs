// Interface for functions systems apply to all their entities.

#ifndef SYS_FUNCS_H
#define SYS_FUNCS_H

#include "sys.h"
#include "cgroup.h"

typedef void (* sys_func_t)(struct CGroup);

// Systems have various functions affecting their entities.
// "enum SysFuncType" is an enumeration representation of
// the different types of system functions.
enum SysFuncType {
        SYS_START,
        SYS_UPDATE,
        SYS_DRAW,
        SYS_DESTROY
};

// Create a system and initialize all its associated/underlying data.
// The start function is given as an argument since it's supposed to
// be called at once when the system is created, so it kind of has to
// exist when the system is created.
struct Sys create_sys(const struct CtSet * requirements, sys_func_t start_func);

void destroy_sys(struct Sys * sys);

// Get the function in 'sys' of type 'type', returning 'NULL' if
// it doesn't exist.
sys_func_t get_sys_func(struct Sys sys, enum SysFuncType type);

// Sets the system function of type "func_type" in "sys" to "func".
// "func" may be NULL, in which case the system won't execute the
// function until it's given a valid value.
// Setting a system function to a value it already has is legal.
// Setting a system function to a non-NULL invalid function
// pointer, or function pointer not compatible with "sys_func_t",
// is illegal.
void set_sys_func(struct Sys sys, enum SysFuncType func_type, sys_func_t func);

#endif
