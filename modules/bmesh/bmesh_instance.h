#pragma once

#include "bmesh.h"
#include "scene/3d/visual_instance_3d.h"

class BMeshInstance3D : public GeometryInstance3D {
	GDCLASS(BMeshInstance3D, GeometryInstance3D);

protected:
	Ref<BMesh> mesh;
	Vector<Ref<Material>> materials;

	void _mesh_changed();

protected:

	bool _set(StringName const& p_name, Variant const& p_value);
	bool _get(StringName const& p_name, Variant& r_ret) const;
	void _get_property_list(List<PropertyInfo>* p_list) const;

	static void _bind_methods();
public:

	void set_mesh(Ref<BMesh> const& p_bmesh);
	Ref<BMesh> get_mesh() const { return mesh; }

	int get_surface_material_count() const;
	void set_surface_material(int p_surface, Ref<Material> const& p_material);
	Ref<Material> get_surface_material(int p_surface) const;
	Ref<Material> get_active_material(int p_surface) const;

	Node* create_trimesh_collision_node();
	void create_trimesh_collision();

	Node* create_convex_collision_node();
	void create_convex_collision();

	// GeometryInstance3D overrides
	virtual AABB get_aabb() const;
	virtual Vector<Face3> get_faces(uint32_t p_usage_flags) const;
	// ~GeometryInstance3D

	BMeshInstance3D();
	~BMeshInstance3D();
};
