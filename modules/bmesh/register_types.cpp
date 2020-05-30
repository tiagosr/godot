#include "register_types.h"

#include "bmesh.h"
#include "bmesh_instance.h"

void register_bmesh_types() {
	ClassDB::register_class<BMeshVertex>();
	ClassDB::register_class<BMeshEdge>();
	ClassDB::register_class<BMeshLoop>();
	ClassDB::register_class<BMeshFace>();
	ClassDB::register_class<BMesh>();
	ClassDB::register_class<BMeshAttributeDefinition>();
	//ClassDB::register_class<BMeshInstance3D>();
}

void unregister_bmesh_types() {
}
