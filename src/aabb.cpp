#include "aabb.h"

//#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

AABB::AABB(short width, short height) :AntiAliasing(width, height)
{
}

AABB::~AABB()
{
}

int AABB::LoadGeometry(std::string filename)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	size_t delimeter = filename.find_last_of("/\\");
	std::string directory = filename.substr(0, delimeter);

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), directory.c_str());

	if (!warn.empty()) {
		std::cout << warn << std::endl;
	}

	if (!err.empty()) {
		std::cerr << err << std::endl;
	}

	if (!ret) {
		exit(1);
	}

	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		Mesh mesh;

		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];

			// Loop over vertices in the face.
			std::vector<Vertex> vertexes;
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				if (idx.normal_index > 0)
				{
					vertexes.push_back(Vertex(float3{
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2] },
					float3{
						attrib.normals[3 * idx.normal_index + 0],
						attrib.normals[3 * idx.normal_index + 1],
						attrib.normals[3 * idx.normal_index + 2]
						}));
				}
				else
				{
					vertexes.push_back(Vertex(float3{
					attrib.vertices[3 * idx.vertex_index + 0],
					attrib.vertices[3 * idx.vertex_index + 1],
					attrib.vertices[3 * idx.vertex_index + 2]
						}));
				}
			}
			index_offset += fv;

			MaterialTriangle triangle(vertexes[0], vertexes[1], vertexes[2]);

			// per-face material
			tinyobj::material_t material = materials[shapes[s].mesh.material_ids[f]];

			triangle.SetEmisive(float3{ material.emission });
			triangle.SetAmbient(float3{ material.ambient });
			triangle.SetDiffuse(float3{ material.diffuse });
			triangle.SetSpecular(float3{ material.specular }, material.shininess);
			triangle.SetReflectiveness(material.illum == 5);
			triangle.SetReflectivenessAndTransparency(material.illum == 7);
			triangle.SetIor(material.ior);

			mesh.AddTriangle(triangle);
		}
		meshes.push_back(mesh);
	}

	return 0;
}

Payload AABB::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	if (max_raytrace_depth <= 0)
	{
		return Miss(ray);
	}

	IntersectableData closestData(t_max);
	MaterialTriangle closestTriangle;

	for (auto& mesh : meshes) {
		if (!mesh.AABBTest(ray)) {
			continue;
		}
		for (auto& object : mesh.Triangles())
		{
			auto data = object.Intersect(ray);
			if (data.t > t_min&& data.t < closestData.t)
			{
				closestData = data;
				closestTriangle = object;
			}
		}
	}

	if (closestData.t < t_max)
	{
		return Hit(ray, closestData, &closestTriangle, max_raytrace_depth);
	}

	return Miss(ray);
}

float AABB::TraceShadowRay(const Ray& ray, const float max_t) const
{
	IntersectableData closestData(max_t);
	for (auto& mesh : meshes)
	{
		if (!mesh.AABBTest(ray))
		{
			continue;
		}
		for (auto& object : mesh.Triangles())
		{
			auto data = object.Intersect(ray);
			if (data.t > t_min&& data.t < closestData.t)
			{
				return data.t;
			}
		}
	}

	return max_t;
}

void Mesh::AddTriangle(const MaterialTriangle triangle)
{
	if (triangles.empty()) {
		aabb_max = aabb_min - triangle.a.position;
	}
	triangles.push_back(triangle);

	aabb_max = max(triangle.a.position, aabb_max);
	aabb_max = max(triangle.b.position, aabb_max);
	aabb_max = max(triangle.c.position, aabb_max);

	aabb_min = min(triangle.a.position, aabb_min);
	aabb_min = min(triangle.b.position, aabb_min);
	aabb_min = min(triangle.c.position, aabb_min);
}

bool Mesh::AABBTest(const Ray& ray) const
{
	float3 invRaydir = float3(1.0) / ray.direction;
	float3 t0 = (aabb_max - ray.position) * invRaydir;
	float3 t1 = (aabb_min - ray.position) * invRaydir;
	float3 tmin = min(t0, t1);
	float3 tmax = max(t0, t1);
	return maxelem(tmin) <= minelem(tmax);
}