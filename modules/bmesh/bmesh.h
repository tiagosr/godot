#pragma once

#include "core/math/math_funcs.h"
#include "core/math/vector3i.h"
#include "core/ustring.h"
#include "scene/resources/mesh.h"

struct BMeshEdge;
struct BMeshLoop;
struct BMeshFace;

struct BMeshVertex : public Reference {
    GDCLASS(BMeshVertex, Reference);

public:
    int id;
    Vector3 point;
    Dictionary attributes;
    Ref<BMeshEdge> edge;

	Vector<Ref<BMeshEdge>> neighbor_edges() const;
	Vector<Ref<BMeshFace>> neighbor_faces() const;

	BMeshVertex() {}
	explicit BMeshVertex(Vector3 const& p)
		: point(p) { }
};

struct BMeshEdge : public Reference {
    GDCLASS(BMeshEdge, Reference);

public:
    int id;
    Dictionary attributes;
    Ref<BMeshVertex> vert1;
    Ref<BMeshVertex> vert2;

    Ref<BMeshEdge> next1;
	Ref<BMeshEdge> next2;

	Ref<BMeshEdge> prev1;
	Ref<BMeshEdge> prev2;

    Ref<BMeshLoop> loop;


	bool contains_vertex(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() || v == vert2.ptr();
	}
	Ref<BMeshVertex> other_vertex(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() ? vert2 : vert1;
	}
	Ref<BMeshEdge> get_next(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() ? next1 : next2;
	}
	void set_next(BMeshVertex const* v, Ref<BMeshEdge> other) {
		CRASH_COND_MSG(v, "v is null!");
		if (v == vert1.ptr()) {
			next1 = other;
		} else {
			next2 = other;
		}
	}

	Ref<BMeshEdge> get_prev(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() ? prev1 : prev2;
	}
	void set_prev(BMeshVertex const* v, Ref<BMeshEdge> other) {
		CRASH_COND_MSG(v, "v is null!");
		if (v == vert1.ptr()) {
			prev1 = other;
		} else {
			prev2 = other;
		}
	}

	Vector<Ref<BMeshFace>> neighbor_faces() const;

	Vector3 center() const {
		return (vert1->point + vert2->point) * 0.5f;
	}

	void set_center(Vector3 const& new_center) {
		Vector3 offset = new_center - center();
		vert1->point += offset;
		vert2->point += offset;
	}

	size_t fill_lines_vector3_array(PackedVector3Array& p_array, size_t p_offset) const;
};

struct BMeshLoop : public Reference {
    GDCLASS(BMeshLoop, Reference);

public:
    Dictionary attributes;

    Ref<BMeshVertex> vert;
    Ref<BMeshEdge> edge;
    Ref<BMeshFace> face; // only exactly one face using a loop

    Ref<BMeshLoop> radial_prev;
    Ref<BMeshLoop> radial_next;

    Ref<BMeshLoop> prev;
    Ref<BMeshLoop> next;

	void set_face(Ref<BMeshFace> f);
	void set_edge(Ref<BMeshEdge> e);

	BMeshLoop() {}

	BMeshLoop(Ref<BMeshVertex> v, Ref<BMeshEdge> e, Ref<BMeshFace> f)
	: vert(v) {
		set_edge(e);
		set_face(f);
	}

	size_t get_loop_edges_count() const;
	size_t fill_lines_vector3_array(PackedVector3Array& p_array, size_t p_offset) const;
};

struct BMeshFace : public Reference {
    GDCLASS(BMeshFace, Reference);

public:
    int id;
    Dictionary attributes;
    int vertcount;
    Ref<BMeshLoop> loop;

	Vector<Ref<BMeshVertex>> neighbor_vertices() const;
	Vector<Ref<BMeshEdge>> neighbor_edges() const;
	Vector3 center() const {
		Vector3 p(0.0f, 0.0f, 0.0f);
		float sum = 0.0f;
		Vector<Ref<BMeshVertex>> vertices = neighbor_vertices();
		for (size_t i = 0; i < vertices.size(); i++) {
			p += vertices[i]->point;
			sum += 1.0f;
		}
		return p / sum;
	}

	size_t get_triangles_count() const {
		switch (vertcount) {
		case 3:
			return 1;
		case 4:
			return 2;
		}
		// unsupported vertcount value
		return 0;
	}
	size_t fill_triangles_vector3_array(PackedVector3Array& p_array, size_t p_offset) const;
};

struct BMeshAttributeDefinition : public Reference {
	GDCLASS(BMeshAttributeDefinition, Reference);

public:
	String name;
	enum Type {
		Int,
		Float
	};

	Type baseType;
	int dimensions;

	Variant defaultValue;

	bool check_value(Variant value) const {
		switch (baseType) {
		case Int:
		{
			switch (dimensions) {
			case 1:
				return value.get_type() == Variant::Type::INT;
			case 2:
				return value.get_type() == Variant::Type::VECTOR2I;
			case 3:
				return value.get_type() == Variant::Type::VECTOR3I;
			default:
				return (value.get_type() == Variant::Type::PACKED_INT32_ARRAY || value.get_type() == Variant::Type::PACKED_INT64_ARRAY);
			}
		}
		case Float:
			switch (dimensions) {
			case 1:
				return value.get_type() == Variant::Type::FLOAT;
			case 2:
				return value.get_type() == Variant::Type::VECTOR2;
			case 3:
				return value.get_type() == Variant::Type::VECTOR3;
			case 4:
				return (value.get_type() == Variant::Type::PACKED_FLOAT32_ARRAY || value.get_type() == Variant::Type::COLOR);
			default:
				return (value.get_type() == Variant::Type::PACKED_FLOAT32_ARRAY);
			}
		}
		return false;
	}

	BMeshAttributeDefinition() {}
	BMeshAttributeDefinition(String const& name, Type baseType, int dimensions)
		: name(name), baseType(baseType), dimensions(dimensions) { }
	BMeshAttributeDefinition(String const& name, Type baseType, int dimensions, Variant defaultValue)
		: name(name), baseType(baseType), dimensions(dimensions), defaultValue(defaultValue) { }
};

class BMesh : public ArrayMesh {
    GDCLASS(BMesh, ArrayMesh);

protected:
	RID rid;
	Mesh::PrimitiveType primitive_type;

	bool flip_faces;

	mutable bool pending_update_request;

	mutable Vector<Ref<BMeshVertex>> vertices;
	mutable Vector<Ref<BMeshEdge>> edges;
	mutable Vector<Ref<BMeshLoop>> loops;
	mutable Vector<Ref<BMeshFace>> faces;

	mutable Vector<Ref<BMeshAttributeDefinition>> vertexAttributes;
	mutable Vector<Ref<BMeshAttributeDefinition>> edgeAttributes;
	mutable Vector<Ref<BMeshAttributeDefinition>> loopAttributes;
	mutable Vector<Ref<BMeshAttributeDefinition>> faceAttributes;

    static void _bind_methods();

	virtual void _create_mesh_arrays(Array arrays);
	void _request_update();
	void _update() const;

	Ref<BMeshVertex> _add_vertex(Variant const& point) { return add_vertex((Vector3)point); }
	Ref<BMeshEdge> _add_edge(Variant const& v1, Variant const& v2) {
		if ((v1.get_type() == Variant::Type::INT) && (v2.get_type() == Variant::Type::INT)) {
			return add_edge((int)v1, (int)v2);
		} else {
			return add_edge(Ref<BMeshVertex>(v1), Ref<BMeshVertex>(v2));
		}
	}

	Variant _add_face(Variant const** p_args, int p_argcount, Callable::CallError& r_error) {
		if ((p_argcount == 1) && p_args[0]->is_array()) {
			Array const& arr(*p_args[0]);
			Vector<Ref<BMeshVertex>> vertices;
			vertices.resize(arr.size());
			for (size_t i = 0, c = arr.size(); i < c; ++i) {
				vertices.write[i] = Ref<BMeshVertex>(arr[i]);
			}
			return add_face(vertices);
		} else if (p_argcount == 3) { // only 3 arguments
			if ((p_args[0]->get_type() == Variant::Type::INT) &&
				(p_args[1]->get_type() == Variant::Type::INT) &&
				(p_args[2]->get_type() == Variant::Type::INT)) {
				return add_face(
					p_args[0]->operator int(),
					p_args[1]->operator int(),
					p_args[2]->operator int());
			} else {
				return add_face(Ref<BMeshVertex>(p_args[0]), Ref<BMeshVertex>(p_args[1]), Ref<BMeshVertex>(p_args[2]));
			}
		} else if (p_argcount == 4) {
			if ((p_args[0]->get_type() == Variant::Type::INT) &&
				(p_args[1]->get_type() == Variant::Type::INT) &&
				(p_args[2]->get_type() == Variant::Type::INT) &&
				(p_args[3]->get_type() == Variant::Type::INT)) {
				return add_face(
					p_args[0]->operator int(),
					p_args[1]->operator int(),
					p_args[2]->operator int(),
					p_args[3]->operator int());
			} else {
				return add_face(
					Ref<BMeshVertex>(p_args[0]),
					Ref<BMeshVertex>(p_args[1]),
					Ref<BMeshVertex>(p_args[2]),
					Ref<BMeshVertex>(p_args[3]));
			}
		}
		return Variant();
	}

public:

	Vector<Face3> collect_faces() const;

	Vector<Ref<BMeshVertex>> const& get_vertices() const { return vertices; }
	Vector<Ref<BMeshEdge>> const& get_edges() const { return edges; }
	Vector<Ref<BMeshLoop>> const& get_loops() const { return loops; }
	Vector<Ref<BMeshFace>> const& get_faces() const { return faces; }

	Vector<Ref<BMeshAttributeDefinition>> const& get_vertex_attributes() const { return vertexAttributes; }
	Vector<Ref<BMeshAttributeDefinition>> const& get_edge_attributes() const { return edgeAttributes; }
	Vector<Ref<BMeshAttributeDefinition>> const& get_loop_attributes() const { return loopAttributes; }
	Vector<Ref<BMeshAttributeDefinition>> const& get_face_attributes() const { return faceAttributes; }

	void set_vertex(int p_idx, Vector3 const& p_vec);
	Vector3 get_vertex(int p_idx) const { return vertices[p_idx]->point; }

	void set_vertex_attribute(int p_idx, String const& p_attr, Variant const& p_value);
	Variant get_vertex_attribute(int p_idx, String const& p_attr) const {
		return vertices[p_idx]->attributes[p_attr];
	}

	Ref<BMeshVertex> add_vertex(Ref<BMeshVertex> vert) {
		ensure_vertex_attributes(vert);
		vertices.append(vert);
		return vert;
	}
	Ref<BMeshVertex> add_vertex(Vector3 point) {
		Ref<BMeshVertex> vert(memnew(BMeshVertex(point)));
		return add_vertex(vert);
	}
	Ref<BMeshVertex> add_vertex(real_t x, real_t y, real_t z) {
		return add_vertex(Vector3(x, y, z));
	}

	Ref<BMeshEdge> add_edge(Ref<BMeshVertex> vert1, Ref<BMeshVertex> vert2);
	Ref<BMeshEdge> add_edge(int v1, int v2) {
		ERR_FAIL_INDEX_V(v1, vertices.size(), nullptr);
		ERR_FAIL_INDEX_V(v2, vertices.size(), nullptr);
		return add_edge(vertices[v1], vertices[v2]);
	}

	Ref<BMeshFace> add_face(Vector<Ref<BMeshVertex>> const& fVerts);
	Ref<BMeshFace> add_face(Ref<BMeshVertex> v0, Ref<BMeshVertex> v1, Ref<BMeshVertex> v2) {
		Vector<Ref<BMeshVertex>> verts;
		verts.push_back(v0);
		verts.push_back(v1);
		verts.push_back(v2);
		return add_face(verts);
	}
	Ref<BMeshFace> add_face(Ref<BMeshVertex> v0, Ref<BMeshVertex> v1, Ref<BMeshVertex> v2, Ref<BMeshVertex> v3) {
		Vector<Ref<BMeshVertex>> verts;
		verts.push_back(v0);
		verts.push_back(v1);
		verts.push_back(v2);
		verts.push_back(v3);
		return add_face(verts);
	}
	Ref<BMeshFace> add_face(int i0, int i1, int i2) {
		ERR_FAIL_INDEX_V(i0, vertices.size(), nullptr);
		ERR_FAIL_INDEX_V(i1, vertices.size(), nullptr);
		ERR_FAIL_INDEX_V(i2, vertices.size(), nullptr);
		return add_face(vertices[i0], vertices[i1], vertices[i2]);
	}
	Ref<BMeshFace> add_face(int i0, int i1, int i2, int i3) {
		ERR_FAIL_INDEX_V(i0, vertices.size(), nullptr);
		ERR_FAIL_INDEX_V(i1, vertices.size(), nullptr);
		ERR_FAIL_INDEX_V(i2, vertices.size(), nullptr);
		ERR_FAIL_INDEX_V(i3, vertices.size(), nullptr);
		return add_face(vertices[i0], vertices[i1], vertices[i2], vertices[i3]);
	}

	Ref<BMeshEdge> find_edge(Ref<BMeshVertex> vert1, Ref<BMeshVertex> vert2);
	void remove_vertex(Ref<BMeshVertex> v);
	void remove_edge(Ref<BMeshEdge> e);
	void remove_loop(Ref<BMeshLoop> l);
	void remove_face(Ref<BMeshFace> f);

	bool has_vertex_attribute(String const& attribName) const;
	bool has_vertex_attribute(Ref<BMeshAttributeDefinition> attrib) const {
		return has_vertex_attribute(attrib->name);
	}
	Ref<BMeshAttributeDefinition> add_vertex_attribute(Ref<BMeshAttributeDefinition> attrib);
	Ref<BMeshAttributeDefinition> add_vertex_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions);

	void ensure_vertex_attributes(Ref<BMeshVertex> v);


	bool has_edge_attribute(String attribName) const;
	bool has_edge_attribute(Ref<BMeshAttributeDefinition> attrib) const {
		return has_edge_attribute(attrib->name);
	}
	Ref<BMeshAttributeDefinition> add_edge_attribute(Ref<BMeshAttributeDefinition> attrib);
	Ref<BMeshAttributeDefinition> add_edge_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions);

	void ensure_edge_attributes(Ref<BMeshEdge> e);


	bool has_loop_attribute(String const& attribName) const;
	bool has_loop_attribute(Ref<BMeshAttributeDefinition> attrib) const {
		return has_loop_attribute(attrib->name);
	}
	Ref<BMeshAttributeDefinition> add_loop_attribute(Ref<BMeshAttributeDefinition> attrib);
	Ref<BMeshAttributeDefinition> add_loop_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions);

	void ensure_loop_attributes(Ref<BMeshLoop> l);


	bool has_face_attribute(String attribName) const;
	bool has_face_attribute(Ref<BMeshAttributeDefinition> attrib) const {
		return has_face_attribute(attrib->name);
	}
	Ref<BMeshAttributeDefinition> add_face_attribute(Ref<BMeshAttributeDefinition> attrib);
	Ref<BMeshAttributeDefinition> add_face_attribute(String name, BMeshAttributeDefinition::Type type, int dimensions);

	void ensure_face_attributes(Ref<BMeshFace> f);

	void set_material(Ref<Material> const& mat);
	Ref<Material> get_material() const;

	// Mesh
	virtual int get_surface_count() const override;
	virtual int surface_get_array_len(int surface) const override;
	virtual int surface_get_array_index_len(int surface) const override;
	virtual Array surface_get_arrays(int surface) const override;
	virtual Array surface_get_blend_shape_arrays(int p_surface) const override;
	virtual Dictionary surface_get_lods(int surface) const override;
	virtual uint32_t surface_get_format(int surface) const override;
	virtual Mesh::PrimitiveType surface_get_primitive_type(int surface) const override;
	virtual Ref<Material> surface_get_material(int surface) const override;
	virtual int get_blend_shape_count() const override;
	virtual StringName get_blend_shape_name(int p_index) const override;
	virtual AABB get_aabb() const override;
	virtual RID get_rid() const override;
	// ~Mesh

	size_t get_num_tris() const;
	size_t generate_tris(PackedVector3Array& p_array, size_t p_offset) const;

    BMesh();
	~BMesh();
};


