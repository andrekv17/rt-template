#include "lighting.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include <algorithm>

Lighting::Lighting(short width, short height) : MTAlgorithm(width, height)
{
}

Lighting::~Lighting()
{
}

int Lighting::LoadGeometry(std::string filename)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    std::string warn;
    std::string err;

    size_t delimiter = filename.find_last_of("/");
    std::string directory = filename.substr(0, delimiter);

    bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), directory.c_str());

    if (!warn.empty()) {
        std::cout << warn << std::endl;
    }

    if (!err.empty()) {
        std::cerr << err << std::endl;
        return 1;
    }

    if (!ret)
    {
        return 1;
    }

    // Loop over shapes
    for (size_t s = 0; s < shapes.size(); s++)
    {
        // Loop over faces(polygon)
        size_t index_offset = 0;
        for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
        {
            int fv = shapes[s].mesh.num_face_vertices[f];

            std::vector <Vertex> vertices;
            // Loop over vertices in the face.
            for (size_t v = 0; v < fv; v++)
            {
                // access to vertex
                tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
                tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
                tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
                tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
                if (idx.normal_index >= 0)
                {
                    tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
                    tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
                    tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
                    vertices.push_back(Vertex(float3{ vx, vy, vz }, float3{ nx, ny, nz }));
                }
                else
                {
                    vertices.push_back(Vertex(float3{ vx, vy, vz }));
                }
            }
            index_offset += fv;

            MaterialTriangle* triangle = new MaterialTriangle(vertices[0], vertices[1], vertices[2]);

            // per-face material
            int material_index = shapes[s].mesh.material_ids[f];
            tinyobj::material_t material = materials[material_index];

            triangle->SetEmisive(float3{ material.emission });
            triangle->SetAmbient(float3{ material.ambient });
            triangle->SetDiffuse(float3{ material.diffuse });
            triangle->SetSpecular(float3{ material.specular }, material.shininess);
            triangle->SetReflectiveness(material.illum == 5);
            triangle->SetReflectivenessAndTransparency(material.illum == 7);
            triangle->SetIor(material.ior);

            material_objects.push_back(triangle);
        }
    }

    return 0;
}

void Lighting::AddLight(Light* light)
{
    lights.push_back(light);
}

Payload Lighting::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
    IntersectableData closest_data(t_max);
    MaterialTriangle* closest_triangle = nullptr;
    for (auto& object : material_objects)
    {
        IntersectableData data = object->Intersect(ray);
        if (data.t > t_min&& data.t < closest_data.t)
        {
            closest_data = data;
            closest_triangle = object;
        }
    }

    if (closest_data.t < t_max)
        return Hit(ray, closest_data, closest_triangle);

    return Miss(ray);
}


Payload Lighting::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle) const
{
    if (triangle == nullptr)
        return Miss(ray);

    Payload payload;

    payload.color = triangle->emissive_color;
    float3 X = ray.position + ray.direction * data.t;
    float3 N = triangle->GetNormal(data.baricentric);
    for (auto light : lights) {
        Ray to_light(X, light->position - X);
        payload.color += light->color * triangle->diffuse_color * std::max(0.0f, dot(N, to_light.direction));
        float3 reflective_direction = 2.0f * dot(N, to_light.direction) * N - to_light.direction;
        payload.color += light->color * triangle->specular_color *
            powf(std::max(0.0f, dot(ray.direction, reflective_direction)), triangle->specular_exponent);
    }


    return payload;
}

float3 MaterialTriangle::GetNormal(float3 barycentric) const
{
    if (length(a.normal) > 0.0f && length(b.normal) > 0.0f && length(c.normal) > 0.0f)
        return a.normal * barycentric.x + b.normal * barycentric.y + c.normal * barycentric.z;
    return geo_normal;
}
