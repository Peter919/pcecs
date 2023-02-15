#ifndef ARCT_EDGES_H
#define ARCT_EDGES_H

#include "../tools/mem_tools.h"
#include "../interface/ct.h"
#include "arct.h"
#include "map.h"

#define ARCT_EDGES_FS "archetype edges (%d initialized)"
#define ARCT_EDGES_FA(arct_edges) (int) (arct_edges).edges.length

struct ArctEdges {
        struct Arct arct;
        struct Map edges;
};

struct ArctEdges create_arct_edges(struct Arct arct);

struct Arct get_edge_with_ct(struct ArctEdges * edges, struct Ct ct);

struct Arct get_edge_without_ct(struct ArctEdges * edges, struct Ct ct);

#endif
