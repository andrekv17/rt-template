#include "shadow_rays.h"

ShadowRays::ShadowRays(short width, short height): Lighting(width, height)
{
}

ShadowRays::~ShadowRays()
{
}

Payload ShadowRays::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
    if (max_raytrace_depth == 0)
        return Miss(ray);
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
        return Hit(ray, closest_data, closest_triangle, max_raytrace_depth);

    return Miss(ray);
}


Payload ShadowRays::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
    if (triangle == nullptr)
        return Miss(ray);

    Payload payload;

    payload.color = triangle->emissive_color;
    float3 X = ray.position + ray.direction * data.t;
    float3 N = triangle->GetNormal(data.baricentric);
    for (auto light : lights) {
        Ray to_light(X, light->position - X);
        float to_light_distance = length(light->position - X);
        float t = TraceShadowRay(to_light, to_light_distance);
        if (fabs(t-to_light_distance) > 0.001f)
            continue;
        payload.color += light->color * triangle->diffuse_color * std::max(0.0f, dot(N, to_light.direction));
        float3 reflective_direction = 2.0f * dot(N, to_light.direction) * N - to_light.direction;
        payload.color += light->color * triangle->specular_color *
            powf(std::max(0.0f, dot(ray.direction, reflective_direction)), triangle->specular_exponent);
    }

    return payload;
}

float ShadowRays::TraceShadowRay(const Ray& ray, const float max_t) const
{
    IntersectableData closest_data(max_t);
    MaterialTriangle* closest_triangle = nullptr;
    for (auto& object : material_objects)
    {
        IntersectableData data = object->Intersect(ray);
        if (data.t > t_min && data.t < closest_data.t)
        {
            return data.t;
        }
    }
    return max_t;
}

