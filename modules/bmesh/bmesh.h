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

	Vector<Ref<BMeshEdge>> NeighborEdges() const;
	Vector<Ref<BMeshFace>> NeighborFaces() const;

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


	bool ContainsVertex(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() || v == vert2.ptr();
	}
	Ref<BMeshVertex> OtherVertex(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() ? vert2 : vert1;
	}
	Ref<BMeshEdge> Next(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() ? next1 : next2;
	}
	void SetNext(BMeshVertex const* v, Ref<BMeshEdge> other) {
		CRASH_COND_MSG(v, "v is null!");
		if (v == vert1.ptr()) {
			next1 = other;
		} else {
			next2 = other;
		}
	}

	Ref<BMeshEdge> Prev(BMeshVertex const* v) const {
		CRASH_COND_MSG(v, "v is null!");
		return v == vert1.ptr() ? prev1 : prev2;
	}
	void SetPrev(BMeshVertex const* v, Ref<BMeshEdge> other) {
		CRASH_COND_MSG(v, "v is null!");
		if (v == vert1.ptr()) {
			prev1 = other;
		} else {
			prev2 = other;
		}
	}

	Vector<Ref<BMeshFace>> NeighborFaces() const;

	Vector3 Center() {
		return (vert1->point + vert2->point) * 0.5f;
	}
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

	void SetFace(Ref<BMeshFace> f);
	void SetEdge(Ref<BMeshEdge> e);

	BMeshLoop() {}

	BMeshLoop(Ref<BMeshVertex> v, Ref<BMeshEdge> e, Ref<BMeshFace> f)
	: vert(v) {
		SetEdge(e);
		SetFace(f);
	}
};

struct BMeshFace : public Reference {
    GDCLASS(BMeshFace, Reference);

public:
    int id;
    Dictionary attributes;
    int vertcount;
    Ref<BMeshLoop> loop;

	Vector<Ref<BMeshVertex>> NeighborVertices() const;
	Vector<Ref<BMeshEdge>> NeighborEdges() const;
	Vector3 Center() const {
		Vector3 p(0.0f, 0.0f, 0.0f);
		float sum = 0.0f;
		Vector<Ref<BMeshVertex>> vertices = NeighborVertices();
		for (size_t i = 0; i < vertices.size(); i++) {
			p += vertices[i]->point;
			sum += 1.0f;
		}
		return p / sum;
	}
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

class BMesh : public Mesh {
    GDCLASS(BMesh, Mesh);

protected:
	RID rid;
	Mesh::PrimitiveType primitive_type;

	Ref<Material> material;

	mutable AABB aabb;
	AABB custom_aabb;

	bool flip_faces;

	mutable size_t array_len;
	mutable size_t index_array_len;

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

	virtual void _create_mesh_array(Array arr);
	void _request_update();
	void _update() const;

public:

	Vector<Ref<BMeshVertex>> const& get_vertices() const { return vertices; }
	Vector<Ref<BMeshEdge>> const& get_edges() const { return edges; }
	Vector<Ref<BMeshLoop>> const& get_loops() const { return loops; }
	Vector<Ref<BMeshFace>> const& get_faces() const { return faces; }

	Vector<Ref<BMeshAttributeDefinition>> const& get_vertex_attributes() const { return vertexAttributes; }
	Vector<Ref<BMeshAttributeDefinition>> const& get_edge_attributes() const { return edgeAttributes; }
	Vector<Ref<BMeshAttributeDefinition>> const& get_loop_attributes() const { return loopAttributes; }
	Vector<Ref<BMeshAttributeDefinition>> const& get_face_attributes() const { return faceAttributes; }

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
	Ref<Material> get_material() const { return material; }

	// Mesh
	virtual int get_surface_count() const override;
	virtual int surface_get_array_len(int surface) const override;
	virtual int surface_get_array_index_len(int surface) const override;
	virtual Array surface_get_arrays(int surface) const override;
	virtual Array surface_get_blend_shape_arrays(int p_surface) const override;
	virtual Dictionary surface_get_lods(int surface) const override;
	virtual uint32_t surface_get_format(int surface) const override;
	virtual Mesh::PrimitiveType surface_get_primitive_type(int surface) const override;
	virtual void surface_set_material(int surface, Ref<Material> const& material) override;
	virtual Ref<Material> surface_get_material(int surface) const override;
	virtual int get_blend_shape_count() const override;
	virtual StringName get_blend_shape_name(int p_index) const override;
	virtual AABB get_aabb() const override;
	virtual RID get_rid() const override;
	// ~Mesh

    BMesh();
	~BMesh();
};


