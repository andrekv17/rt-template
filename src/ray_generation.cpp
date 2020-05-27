#include "ray_generation.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

RayGenerationApp::RayGenerationApp(short width, short height) :
	width(width),
	height(height)
{
}

RayGenerationApp::~RayGenerationApp()
{
}

void RayGenerationApp::SetCamera(float3 position, float3 direction, float3 approx_up)
{
	camera.SetPosition(position);
	camera.SetDirection(direction);
	camera.SetUp(approx_up);
	camera.SetRenderTargetSize(width, height);
}

void RayGenerationApp::Clear()
{
	frame_buffer.resize(static_cast<rsize_t>(width) * static_cast<rsize_t>(height));
}

void RayGenerationApp::DrawScene()
{
	for (short x = 0; x < width; x++)
	{
#pragma omp parallel for
		for (short y = 0; y < height; y++)
		{
			Ray ray = camera.GetCameraRay(x, y);
			Payload payload = TraceRay(ray, raytracing_depth);
			SetPixel(x, y, payload.color);
		}
	}
}

int RayGenerationApp::Save(std::string filename) const
{
	int result = stbi_write_png(filename.c_str(), width, height, 3, frame_buffer.data(), width * 3);
	if (result == 1)
	{
		system((std::string("start ") + filename).c_str());
	}
	return 1 - result;
}

Payload RayGenerationApp::TraceRay(const Ray& ray, const unsigned int max_raytrace_depth) const
{
	return Miss(ray);
}

Payload RayGenerationApp::Miss(const Ray& ray) const
{
	Payload payload;
	float t = 0.5f * (ray.direction.y + 1.0f);
	//payload.color = { 0.0f, 0.2f, 0.7f + 0.3f * t };
	payload.color = { (ray.direction.x + 1.0f) * 0.5f, t, 0.5f };
	return payload;
}

void RayGenerationApp::SetPixel(unsigned short x, unsigned short y, float3 color)
{
	byte3 byteColor{ static_cast<uint8_t>(255.0f * color.x),
		static_cast<uint8_t>(255.0f * color.y),
		static_cast<uint8_t>(255.0f * color.z) };
	frame_buffer[y * width + x] = byteColor;
}

Camera::Camera()
{

}

Camera::~Camera()
{
}

void Camera::SetPosition(float3 position)
{
	this->position = position;
}

void Camera::SetDirection(float3 direction)
{
	this->direction = normalize(direction - position);
}

void Camera::SetUp(float3 approx_up)
{
	this->right = normalize(cross(this->direction, normalize(approx_up)));
	this->up = normalize(cross(this->right, this->direction));
}

void Camera::SetRenderTargetSize(short width, short height)
{
	this->width = width; this->height = height;
}

Ray Camera::GetCameraRay(short x, short y) const
{
	float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
	float u = aspectRatio * (2.0f * (static_cast<float>(x) + 0.5f) / static_cast<float>(width) - 1.0f);
	float v = 2.0f * (static_cast<float>(y) + 0.5f) / static_cast<float>(height) - 1.0f;
	float3 direction = this->direction + u * this->right - v * this->up;
	return Ray(this->position, direction);
	return Ray(this->position, direction);
}

Ray Camera::GetCameraRay(short x, short y, float3 jitter) const
{
	return GetCameraRay(x, y);
}
