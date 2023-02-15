// Systems control entities that include a certain set of
// components (the system requirements).

// Systems may have none, any or all of the following methods:
// *    Start: Called when an entity starts matching the system's
//      requirements (when the entity adds components that makes
//      it match the system or the system is created).
// *    Update: Called once a frame for each entity in the system.
// *    Draw: Called once a frame for each entity in the system,
//      after update.
// *    Destroy: Called when an entity is destroyed.

// I imagine "SYS_H" could be a common header macro thing, so we
// prefix it with "PCECS_" to avoid possible name collisions.
#ifndef PCECS_SYS_H
#define PCECS_SYS_H

#include "../ids/id.h"
#include "ct_set.h"

#define SYS_FS "system (" PCECS_ID_FS ")"
#define SYS_FA(sys) PCECS_ID_FA((sys).id)

struct Sys {
        pcecs_id_t id;
};

// "create_sys" is defined in the source file of this header file, but
// declared in "sys_funcs.h" to avoid circular dependency.
// 'But circular dependency is a sign of bad design!' Woah, I didn't
// know that! I'll just decircularize the dependencies by not
// including any headers ever!!! Yay.

// Destroy a system and its associated/underlying data.
void destroy_sys(struct Sys * sys);

// Checks if two systems are the same.
// Returns false if "sys1" and "sys2" are two separate systems,
// even if all of their underlying data (functions, requirements,
// et cetera) are equal.
bool sys_equal(struct Sys sys1, struct Sys sys2);

// Call all update functions on all entities!
void update_entities(void);

// Call all draw functions on all entities!
void draw_entities(void);

#endif
