#include "register_types.h"

#include "bmesh.h"
#include "bmesh_instance.h"
#include "bmesh_gizmos.h"

void register_bmesh_types() {
	ClassDB::register_class<BMeshVertex>();
	ClassDB::register_class<BMeshEdge>();
	ClassDB::register_class<BMeshLoop>();
	ClassDB::register_class<BMeshFace>();
	ClassDB::register_class<BMesh>();
	ClassDB::register_class<BMeshAttributeDefinition>();

#ifdef TOOLS_ENABLED
	EditorPlugins::add_by_type<EditorPluginBMesh>();
#endif
}

void unregister_bmesh_types() {
}
