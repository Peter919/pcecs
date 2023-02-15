#include "entity.h"
#include "../globals/id_mgrs.h"
#include "../tools/mem_tools.h"
#include "../tools/log.h"
#include "../globals/maps.h"
#include "ct_set.h"
#include "../structs/arct.h"
#include "../structs/entity_data.h"
#include "../structs/arct_data.h"

#define CHECK_ENTITY_EXISTENCE(entity, err_return_val) \
        ASSERT_OR_HANDLE(entity_exists(entity), err_return_val, \
                "Non-existent " ENTITY_FS ".", ENTITY_FA(entity));

static bool ct_exists(struct Ct ct)
{
        return map_contains(&g_ct_map, ct.id);
}

#define CHECK_CT_EXISTENCE(ct, err_return_val) \
        ASSERT_OR_HANDLE(ct_exists(ct), err_return_val, \
                "Non-existent " CT_FS ".", CT_FA(ct));

struct Entity create_entity(void)
{
        LOG_DEBUG("Creating entity ...\n");
        struct Entity entity = {
                .id = generate_id_of_type(ID_MGR_ENTITIES)
        };

        // Create an empty "struct CtSet" (entities are initialized with
        // no components), and find an archetype matching that empty set,
        // and therefore also the created entity.
        struct CtSet ct_set = create_ct_set();
        struct Arct arct = create_arct(&ct_set);
        destroy_ct_set(&ct_set);

        // Create underlying data for this entity and add it to the map
        // of all entities.
        struct EntityData entity_data = create_entity_data(arct);
        add_to_map(&g_entity_map, entity.id, &entity_data);

        // Add this entity to the "struct CTable" of its archetype.
        struct ArctData * arct_data = get_map_element(&g_arct_map, arct.id);
        add_entity_to_table(&arct_data->ctable, entity);

        LOG_INFO("Created " ENTITY_FS ".\n", ENTITY_FA(entity));
        LOG_DEBUG_HIDE_LEVEL("\n");

        return entity;
}

static bool entity_exists(struct Entity entity)
{
        return map_contains(&g_entity_map, entity.id);
}

void destroy_entity(struct Entity * entity)
{
        // Return nothing if it doesn't exist.
        CHECK_ENTITY_EXISTENCE(*entity, );

        LOG_INFO("Destroying " ENTITY_FS " ...\n", ENTITY_FA(*entity));

        const struct EntityData * entity_data = get_map_element(&g_entity_map, entity->id);
        struct ArctData * arct_data;
        arct_data = get_map_element(&g_arct_map, entity_data->arct.id);

        // Call the "SYS_DESTROY" functions of all systems affecting "entity".
        struct CGroup cgroup;
        cgroup.entity = *entity;
        for (size_t i = 0; i < arct_data->systems.len; ++i) {

                cgroup.sys.id = arct_data->systems.contents[i];
                sys_func_t sys_destructor = get_sys_func(cgroup.sys, SYS_DESTROY);
                sys_destructor(cgroup);
        }

        // Erase everyone's memory of "entity" so they forger.
        struct CTable * table = &arct_data->ctable;
        destroy_table_entity(table, *entity);
        remove_from_map(&g_entity_map, entity->id);
        destroy_id_of_type(ID_MGR_ENTITIES, entity->id);
}

bool entities_equal(struct Entity entity1, struct Entity entity2)
{
        return entity1.id == entity2.id;
}

bool contains_component(struct Entity entity, struct Ct ct)
{
        // Return false if they don't exist.
        CHECK_ENTITY_EXISTENCE(entity, false);
        CHECK_CT_EXISTENCE(ct, false);

        // Get the component type set of this entity through its archetype.
        const struct EntityData * entity_data = get_map_element(&g_entity_map, entity.id);
        struct Arct arct = entity_data->arct;
        const struct ArctData * arct_data;
        arct_data = get_map_element(&g_arct_map, arct.id);
        const struct CtSet * ct_set = &arct_data->ct_set;

        // Return whether or not the component type set matching the
        // entity contains "ct".
        return ct_in_set(ct_set, ct);
}

static void add_or_remove_component(struct Entity entity, struct Ct ct, bool add)
{
        struct EntityData * entity_data = get_map_element(&g_entity_map, entity.id);

        struct Arct arct = entity_data->arct;
        struct ArctData * archetype_data = get_map_element(&g_arct_map, arct.id);

        // "struct ArctEdges" contain archetypes with the exact same
        // component types as the archetype owning the edges, except
        // they either lack a component type or have an extra.
        // They are accessed using the "get_edge_with_ct" and
        // "get_edge_without_ct" functions, which both have the same
        // signature. Therefore, we can use a pointer to one of the
        // functions to access an archetype either containing or lacking
        // a component that should be added or removed.
        struct Arct (* edge_accessor)(struct ArctEdges *, struct Ct);
        edge_accessor = add ? get_edge_with_ct : get_edge_without_ct;

        // Use "edge_accessor" to find or create an archetype that now
        // matches this entity.
        struct Arct new_arct = (*edge_accessor)(&archetype_data->edges, ct);

        struct ArctData * new_arct_data;
        new_arct_data = get_map_element(&g_arct_map, new_arct.id);
        struct CTable * new_ctable = &new_arct_data->ctable;

        // "edge_accessor" may have created a new archetype, possibly
        // invalidating "archetype_data".
        // Therefore, we must reassign it.
        archetype_data = get_map_element(&g_arct_map, arct.id);
        struct CTable * ctable = &archetype_data->ctable;

        // Move this entity to the component table of the new archetype.
        entity_data->arct = new_arct;
        move_entity(new_ctable, ctable, entity);
}

static void call_start_functions(
        const struct IdPool * systems,
        const struct IdPool * excluded_systems,
        struct Entity entity)
{
        struct CGroup cgroup;
        cgroup.entity = entity;
        for (size_t i = 0; i < systems->len; ++i) {
                struct Sys sys;
                sys.id = systems->contents[i];

                if (!id_in_pool(excluded_systems, sys.id)) {
                        cgroup.sys = sys;
                        sys_func_t start_func = get_sys_func(sys, SYS_START);

                        start_func(cgroup);
                }
        }
}

void add_component(struct Entity entity, struct Ct ct)
{
        // Return nothing if they don't exist.
        CHECK_ENTITY_EXISTENCE(entity, );
        CHECK_CT_EXISTENCE(ct, );

        // Return nothing if condition is false.
        ASSERT_OR_HANDLE(!contains_component(entity, ct), , "Already " CT_FS " in " ENTITY_FS ".",
                CT_FA(ct), ENTITY_FA(entity));

        LOG_INFO("Adding " CT_FS " to " ENTITY_FS " ...\n",
                CT_FA(ct), ENTITY_FA(entity));

        const struct EntityData * entity_data = get_map_element(&g_entity_map, entity.id);
        struct Arct old_arct = entity_data->arct;

        // Call "add_or_remove_component" in addition mode.
        add_or_remove_component(entity, ct, true);

        // The archetype map may have been moved after "add_or_remove_component",
        // so "old_arct_data" cannot be found before after that function has
        // been called.
        const struct ArctData * old_arct_data = get_map_element(&g_arct_map, old_arct.id);

        struct Arct new_arct = entity_data->arct;
        const struct ArctData * new_arct_data = get_map_element(&g_arct_map, new_arct.id);

        call_start_functions(&new_arct_data->systems, &old_arct_data->systems, entity);

        LOG_DEBUG_HIDE_LEVEL("\n");
}

void remove_component(struct Entity entity, struct Ct ct)
{
        // Return nothing if they don't exist.
        // Return nothing if they don't exist.
        // Return nothing if they don't exist.
        CHECK_ENTITY_EXISTENCE(entity, );
        CHECK_CT_EXISTENCE(ct, );

        // Return nothing if condition is false.
        ASSERT_OR_HANDLE(contains_component(entity, ct), , "No " CT_FS " in " ENTITY_FS ".",
                CT_FA(ct), ENTITY_FA(entity));

        LOG_INFO("Removing " CT_FS " from " ENTITY_FS " ...\n",
                CT_FA(ct), ENTITY_FA(entity));

        // Call "add_or_remove_component" in removal mode.
        add_or_remove_component(entity, ct, false);
}

void * get_component_from_entity(struct Entity entity, struct Ct ct)
{
        CHECK_ENTITY_EXISTENCE(entity, NULL);
        CHECK_CT_EXISTENCE(ct, NULL);

        ASSERT_OR_HANDLE(contains_component(entity, ct), NULL, "No " CT_FS " in " ENTITY_FS ".",
                CT_FA(ct), ENTITY_FA(entity));


        // Retrieve the component table containing "entity" through a
        // series of steps, before returning a component found in that
        // table.
        const struct EntityData * entity_data = get_map_element(&g_entity_map, entity.id);
        const struct ArctData * arct_data;
        arct_data = get_map_element(&g_arct_map, entity_data->arct.id);
        return get_table_component(&arct_data->ctable, entity, ct);
}
