// Global maps converting ids to actual data.

#ifndef MAPS_H
#define MAPS_H

#include "../structs/map.h"

struct Map g_entity_map;
struct Map g_ct_map;
struct Map g_sys_map;
struct Map g_arct_map;

// Initialize all global maps. Must only be done once.
void init_maps(void);

#endif
