// The underlying data of component types.
// IDs of component types are mapped to this structure.
// More explanation on what component types are for can be found
// in "ct.h"
// Component type IDs are mapped to these structs in "g_ct_map".

#ifndef CT_DATA_H
#define CT_DATA_H

#include "arct.h"
#include "../ids/id_pool.h"

#define CT_DATA_FS "component type data%s"
#define CT_DATA_FA(ct_data) ""

struct CtData {
        // The archetypes whose entities contains a component of
        // this type.
        struct IdPool arcts;
        // The size of a component of this type, in bytes.
        size_t size;
        // The method used to destroy instances of this type.
        // "component" is a pointer to the component.
        void (* destructor)(void * component);
};

// Creates a new "CtData" structure, where each instance has size
// "size" and is destroyed by "destructor".
struct CtData create_ct_data(size_t size, void (* destructor)(void * component));

// Destroys a "struct CtData".
// So far, calling it is not legal since the only reason to
// destroy a component type is to free space, which isn't
// necessary unless there's a huge number of unused compoent
// types.
void destroy_ct_data(struct CtData * ct_data);

// Same as "destroy_ct_data", but the argument is a void pointer
// so it can be used by generalized function pointers.
void destroy_ct_data_void(void * ct_data);

// Makes "ct" aware that entities of "arct" contains a component
// of that type.
// Cannot be called unless "arct" contains "ct", as described above.
// Must be called shortly after "arct" is created.
void add_arct_to_ct(struct CtData * ct, struct Arct arct);

#endif
