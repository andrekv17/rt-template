#include "refraction.h"

Refraction::Refraction(short width, short height) :Reflection(width, height)
{
	raytracing_depth = 3;
}

Refraction::~Refraction()
{
}

Payload Refraction::Hit(const Ray& ray, const IntersectableData& data, const MaterialTriangle* triangle, const unsigned int max_raytrace_depth) const
{
	if (triangle == nullptr)
	{
		return Miss(ray);
	}
	Payload payload;
	payload.color = triangle->emissive_color;

	float3 X = ray.position + ray.direction * data.t;
	float3 N = triangle->GetNormal(data.baricentric);

	if (triangle->reflectiveness)
	{
		Ray reflection_ray(X, ray.direction - 2.f * dot(N, ray.direction) * N);
		return TraceRay(reflection_ray, max_raytrace_depth - 1);
	}

	if (triangle->reflectiveness_and_transparency)
	{
		float kr = 1.f;
		float cosI = std::max(-1.f, std::min(1.f, dot(ray.direction, N)));
		float etaI = 1.f;
		float etaO = triangle->ior;
		if (cosI > 0.f)
		{
			std::swap(etaI, etaO);
		}

		float sinO = etaI / etaO * sqrtf(std::max(0.f, 1 - cosI * cosI));
		if (sinO < 1.f)
		{
			float cosO = sqrtf(std::max(0.f, 1.f - sinO * sinO));
			float cosI = fabs(cosI);
			float Rs = ((etaO * cosI) - (etaI * cosO)) / ((etaO * cosI) + (etaI * cosO));
			float Rp = ((etaI * cosI) - (etaO * cosO)) / ((etaI * cosI) + (etaO * cosO));
			kr = (Rs * Rs + Rp * Rp) / 2.f;
		}

		bool outside = dot(ray.direction, N) < 0;
		float3 bias = 0.001f * N;
		Payload refractionPayload;

		if (kr < 1.f)
		{
			float cosI = std::max(-1.f, std::min(1.f, dot(ray.direction, N)));
			float etaI = 1.f;
			float etaO = triangle->ior;
			if (cosI > 0.f)
			{
				std::swap(etaI, etaO);
			}
			else
			{
				cosI = -cosI;
			}
			cosI = fabs(cosI);
			float eta = etaI / etaO;
			float k = 1.f - eta * eta * (1.f - cosI * cosI);
			float3 refractionDirection{ 0, 0, 0 };
			if (k >= 0.f)
			{
				refractionDirection = eta * ray.direction + (eta * cosI - sqrtf(k)) * N;
			}
			Ray refractionRay(outside ? X - bias : X + bias, refractionDirection);
			refractionPayload = TraceRay(refractionRay, max_raytrace_depth - 1);
		}

		Ray reflectionRay(outside ? X + bias : X - bias, ray.direction - 2.f * dot(N, ray.direction) * N);
		Payload reflectionPayload = TraceRay(reflectionRay, max_raytrace_depth - 1);

		Payload summary;
		summary.color = reflectionPayload.color * kr + refractionPayload.color * (1.f - kr);
		return summary;
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

		// Diffuse
		payload.color += light->color * triangle->diffuse_color
			* std::max(dot(N, toLight.direction), 0.f);

		// Specular
		float3 reflectionDirection = 2.f * dot(N, toLight.direction) * N - toLight.direction;
		payload.color += light->color * triangle->specular_color
			* powf(std::max(dot(ray.direction, reflectionDirection), 0.f), triangle->specular_exponent);
	}
	return payload;
}