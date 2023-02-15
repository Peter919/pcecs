#include "arct_edges.h"
#include "../interface/ct.h"
#include "arct.h"
#include "../tools/log.h"
#include "../tools/debug.h"
#include "../globals/maps.h"
#include "arct_data.h"

struct ArctEdges create_arct_edges(struct Arct arct)
{
        LOG_DEBUG("Creating archetype edges ...\n");

        struct ArctEdges edges;

        edges.arct = arct;
        // Create a map converting (component type) ids to archetypes,
        // initialized with no elements.
        // The map will convert component types to archetypes with the
        // exact same component types as this one, except that the
        // component type used as a key in the map is toggled (if
        // "edges.arct" contains the component, the archetype in the
        // map doesn't and vice versa).
        // Elements of the map are created as needed (lazy evaluation)
        // and provides a fast way to find archetypes with a set of
        // component types very similar to this one's.
        // "noop" is passed as an argument for the destructor of map
        // values. That's done to preserve archetypes referenced in
        // the map even if the map is destroyed.
        edges.edges = create_map(sizeof(struct Arct), NULL);

        LOG_DEBUG("Created " ARCT_EDGES_FS ".\n", ARCT_EDGES_FA(edges));
        return edges;
}

// I might write a faster function for this later, working
// with the underlying implementation of "struct CtSet".
// However, a small optimization like that isn't really
// necessary (right now, at least) and messing with the
// scary secrets of "struct CtSet" would be bad in one of
// three ways:
// 1.   If I wrote a prototype for the function in
//      "ct_set.h" and defined it in the matching
//      ".c"-file, I would contaminate the interface of pcecs,
//      since that file is included externally.
// 2.   If I wrote it here and messed with the implementation,
//      I would violate principles of encapsulation and
//      abstraction.
// 3.   If I wrote it in the source file implementing "struct
//      CtSet" and made a prototype here, I think it would
//      somehow mess up encapsulation or kalafalagation, and
//      I just don't like it.
static void toggle_ct_in_set(struct CtSet * set, struct Ct ct)
{
        if (ct_in_set(set, ct)) {
                remove_ct_from_set(set, ct);
        } else {
                add_ct_to_set(set, ct);
        }
}

static struct ArctEdges * get_arct_edges(struct Arct arct)
{
        struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);
        return &arct_data->edges;
}

static struct Arct create_edge(struct ArctEdges * edges, struct Ct toggled_ct)
{
        LOG_DEBUG("Creating archetype edge ...\n");

        // The archetype data of "edges->arct" includes the set
        // of component types that entities of that entities of
        // that archetype have.
        const struct ArctData * arct_data = get_map_element(&g_arct_map, edges->arct.id);

        // Create a "CtSet" with the same component types as
        // "edges->arct", except if the set contains "toggled_ct",
        // it's removed; otherwise it's added.
        struct CtSet edge_ct_set = create_ct_set();
        copy_ct_set(&edge_ct_set, &arct_data->ct_set);
        toggle_ct_in_set(&edge_ct_set, toggled_ct);

        // Find or create an archetype matching the newly created
        // component type set.
        // "edges" may be invalidated since it's pointing to a
        // place in the archetype map, which may be reallocated
        // when a new archetype is created. Therefore, we must
        // save the archetype that "edges" belongs to and get
        // the correct pointer to "edges" after disturbing the
        // archetype map.
        struct Arct arct = edges->arct;

        struct Arct edge_arct = create_arct(&edge_ct_set);
        destroy_ct_set(&edge_ct_set);

        edges = get_arct_edges(arct);

        // Add the newly found or created archetype to "edges->
        // edges" to make future lookups faster. The changed
        // component type is the key for the map element.
        add_to_map(&edges->edges, toggled_ct.id, &edge_arct);

        return edge_arct;
}

// Get an archetype with the same components as "edges->arct",
// except if the "edges->arct" contains "toggled_ct", the returned
// archetype does not and vice versa.
static struct Arct get_edge(struct ArctEdges * edges, struct Ct toggled_ct)
{
        // If an edge where "toggled_ct" is toggled is already initialized,
        // return that one.
        if (map_contains(&edges->edges, toggled_ct.id)) {
                LOG_DEBUG("Finding archetype edge ...\n");
                const struct Arct * found_edge = get_map_element(&edges->edges, toggled_ct.id);
                return *found_edge;
        }

        // If there's no edge where "toggled_ct" is toggled, use a slower
        // lookup method and create an edge to speed up future lookups.
        return create_edge(edges, toggled_ct);
}

#ifdef ASSERTIONS
// Returns "true" if entities of "arct" contains a component of
// type "ct", otherwise "false".
static bool ct_in_arct(struct Arct arct, struct Ct ct)
{
        const struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);
        const struct CtSet * ct_set = &arct_data->ct_set;
        return ct_in_set(ct_set, ct);
}
#endif

struct Arct get_edge_with_ct(struct ArctEdges * edges, struct Ct ct)
{
        LOG_DEBUG("Getting edge with " CT_FS " in " ARCT_EDGES_FS " ...\n",
                CT_FA(ct), ARCT_EDGES_FA(*edges));

        ASSERT(!ct_in_arct(edges->arct, ct),
                "Already " CT_FS " in " ARCT_FS ".", CT_FA(ct), ARCT_FA(edges->arct));

        // The archetype returned by "get_edge" will contain "ct" if
        // "edges->arct" doesn't and vice versa. "get_edge_with_ct" is
        // only legal for archetypes that doesn't already contain "ct"
        // anyway, so "get_edge" is guaranteed to return an edge that
        // does contain "ct".
        struct Arct edge = get_edge(edges, ct);

        LOG_DEBUG("Got edge " ARCT_FS ".\n", ARCT_FA(edge));
        return edge;
}

struct Arct get_edge_without_ct(struct ArctEdges * edges, struct Ct ct)
{
        LOG_DEBUG("Getting edge without " CT_FS " in " ARCT_EDGES_FS " ...\n",
                CT_FA(ct), ARCT_EDGES_FA(*edges));

        ASSERT(ct_in_arct(edges->arct, ct),
                "No " CT_FS " in " ARCT_FS ".", CT_FA(ct), ARCT_FA(edges->arct));

        // The logic for using "get_edge" is exactly the same as in
        // "get_edge_with_ct", so look there for an explanation (unless
        // the comment is removed, in which case I'm deeply sorry and
        // shallowly humored).
        struct Arct edge = get_edge(edges, ct);

        LOG_DEBUG("Got edge " ARCT_FS ".\n", ARCT_FA(edge));
        return edge;
}
