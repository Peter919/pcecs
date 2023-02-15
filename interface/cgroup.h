// The structure used in system functions ("struct SysFuncs").
// Component groups are really just like entities except
// that you're not allowed to access components that aren't
// required by the system they belong to.
// If you for some reason have to access components not in
// the system requirements, you can access them through the
// "entity" member of the "CGroup" struct.

#ifndef CGROUP_H
#define CGROUP_H

#include "entity.h"
#include "sys.h"

#define CGROUP_FS "component group (" ENTITY_FS ", " SYS_FS ")"
#define CGROUP_FA(cgroup) \
        ENTITY_FA((cgroup).entity), \
        SYS_FA((cgroup).sys)

struct CGroup {
        struct Entity entity;
        struct Sys sys;
};

// Creation and destruction of component groups is done in TODO: [file where cgroups are created and
// destroyed]. That's because they're only supposed to be used by the implementation, and should not
// be exposed to the interface like this file is.

// Asserts that the system of "cgroup" requires its entities to have "ct" before returning the
// component. Otherwise, it's equivalent to "get_component_from_entity(cgroup->entity)".
void * get_component(struct CGroup * cgroup, struct Ct ct);

#endif
