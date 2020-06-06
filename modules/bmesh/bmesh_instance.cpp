#include "bmesh_instance.h"
#include "core/core_string_names.h"
#include "scene/3d/collision_shape_3d.h"
#include "scene/3d/physics_body_3d.h"
#include "scene/resources/material.h"


void BMeshInstance3D::_mesh_changed() {
	materials.resize(mesh->get_surface_count());
}

bool BMeshInstance3D::_set(StringName const & p_name, Variant const & p_value) {
	if (!get_instance().is_valid()) {
		return false;
	}

	if (p_name.operator String().begins_with("material/")) {
		int idx = p_name.operator String().get_slicec('/', 1).to_int();
		if (idx >= materials.size() || idx < 0) {
			return false;
		}

		set_surface_material(idx, p_value);
		return true;
	}
	
	return false;
}

bool BMeshInstance3D::_get(StringName const & p_name, Variant & r_ret) const {
	if (!get_instance().is_valid()) {
		return false;
	}

	if (p_name.operator String().begins_with("material/")) {
		int idx = p_name.operator String().get_slicec('/', 1).to_int();
		if (idx >= materials.size() || idx < 0) {
			return false;
		}
		r_ret = materials[idx];
		return true;
	}
	return false;
}

void BMeshInstance3D::_get_property_list(List<PropertyInfo>* p_list) const {
	if (mesh.is_valid()) {
		for (int i = 0; i < mesh->get_surface_count(); i++) {
			p_list->push_back(PropertyInfo(Variant::OBJECT, "material/" + itos(i), PROPERTY_HINT_RESOURCE_TYPE, "ShaderMaterial,StandardMaterial3D", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_DEFERRED_SET_RESOURCE));
		}
	}
}

void BMeshInstance3D::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_mesh", "mesh"), &BMeshInstance3D::set_mesh);
	ClassDB::bind_method(D_METHOD("get_mesh"), &BMeshInstance3D::get_mesh);

	ClassDB::bind_method(D_METHOD("get_surface_material_count"), &BMeshInstance3D::get_surface_material_count);
	ClassDB::bind_method(D_METHOD("set_surface_material", "surface", "material"), &BMeshInstance3D::set_surface_material);
	ClassDB::bind_method(D_METHOD("get_surface_material", "surface"), &BMeshInstance3D::get_surface_material);
	ClassDB::bind_method(D_METHOD("get_active_material", "surface"), &BMeshInstance3D::get_active_material);

	ClassDB::bind_method(D_METHOD("create_trimesh_collision"), &BMeshInstance3D::create_trimesh_collision);
	ClassDB::set_method_flags("MeshInstance3D", "create_trimesh_collision", METHOD_FLAGS_DEFAULT);
	ClassDB::bind_method(D_METHOD("create_convex_collision"), &BMeshInstance3D::create_convex_collision);
	ClassDB::set_method_flags("MeshInstance3D", "create_convex_collision", METHOD_FLAGS_DEFAULT);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "mesh", PROPERTY_HINT_RESOURCE_TYPE, "BMesh"), "set_mesh", "get_mesh");

}

void BMeshInstance3D::set_mesh(Ref<BMesh> const & p_bmesh) {
	if (mesh == p_bmesh) {
		return;
	}

	if (mesh.is_valid()) {
		mesh->disconnect(CoreStringNames::get_singleton()->changed, callable_mp(this, &BMeshInstance3D::_mesh_changed));
	}

	mesh = p_bmesh;

	if (mesh.is_valid()) {
		mesh->connect(CoreStringNames::get_singleton()->changed, callable_mp(this, &BMeshInstance3D::_mesh_changed));
		materials.resize(mesh->get_surface_count());

		set_base(mesh->get_rid());
	} else {
		set_base(RID());
	}

	update_gizmo();
	_change_notify();
}

int BMeshInstance3D::get_surface_material_count() const {
	return materials.size();
}

void BMeshInstance3D::set_surface_material(int p_surface, Ref<Material> const & p_material) {
	ERR_FAIL_INDEX(p_surface, materials.size());

	materials.write[p_surface] = p_material;

	if (materials[p_surface].is_valid()) {
		RS::get_singleton()->instance_set_surface_material(get_instance(), p_surface, materials[p_surface]->get_rid());
	} else {
		RS::get_singleton()->instance_set_surface_material(get_instance(), p_surface, RID());
	}
}

Ref<Material> BMeshInstance3D::get_surface_material(int p_surface) const {
	ERR_FAIL_INDEX_V(p_surface, materials.size(), nullptr);
	return materials[p_surface];
}

Ref<Material> BMeshInstance3D::get_active_material(int p_surface) const {
	Ref<Material> material_override = get_material_override();
	if (material_override.is_valid()) {
		return material_override;
	}

	Ref<Material> surface_material = get_surface_material(p_surface);
	if (surface_material.is_valid()) {
		return surface_material;
	}

	Ref<Mesh> mesh = get_mesh();
	if (mesh.is_valid()) {
		return mesh->surface_get_material(p_surface);
	}

	return Ref<Material>();
}

Node * BMeshInstance3D::create_trimesh_collision_node() {
	return nullptr;
}

void BMeshInstance3D::create_trimesh_collision() {
}

Node * BMeshInstance3D::create_convex_collision_node() {
	return nullptr;
}

void BMeshInstance3D::create_convex_collision() {
}

AABB BMeshInstance3D::get_aabb() const {
	if (mesh.is_valid()) {
		return mesh->get_aabb();
	}
	return AABB();
}

BMeshInstance3D::BMeshInstance3D() {
}

BMeshInstance3D::~BMeshInstance3D() {
}
