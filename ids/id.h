// A small interface for IDs.

#ifndef PCECS_ID_H
#define PCECS_ID_H

#define PCECS_ID_FS "id %d"
#define PCECS_ID_FA(id) (int) id

// Must not be changed since code might rely on this value.
#define PCECS_INVALID_ID 0

typedef unsigned int pcecs_id_t;

#endif
