#include "reflection.h"

Reflection::Reflection(short width, short height) :ShadowRays(width, height)
{
}

Reflection::~Reflection()
{
}

Payload Reflection::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int raytrace_depth) const
{
	Payload payload;
	payload.color = triangle->emissive_color;

	if (triangle == nullptr)
	{
		return Miss(ray);
	}

	float3 X = ray.position + ray.direction * data.t;
	float3 N = triangle->GetNormal(data.baricentric);

	if (triangle->reflectiveness) {
		float3 reflection_direction = ray.direction - 2.0f * dot(N, ray.direction) * N;
		Ray reflection_ray(X + reflection_direction * 0.001f, reflection_direction);
		return TraceRay(reflection_ray, raytrace_depth - 1);
	}

	for (auto light : lights)
	{
		Ray toLight(X, light->position - X);
		float toLightDistance = length(light->position - X);
		float t = TraceShadowRay(toLight, toLightDistance);
		if (fabs(t - toLightDistance) > t_min)
		{
			continue;
		}

		payload.color += light->color * triangle->diffuse_color
			* std::max(dot(N, toLight.direction), 0.f);

		float3 reflectionDirection = 2.f * dot(N, toLight.direction) * N - toLight.direction;
		payload.color += light->color * triangle->specular_color
			* powf(std::max(dot(ray.direction, reflectionDirection), 0.f), triangle->specular_exponent);
	}

	return payload;
}