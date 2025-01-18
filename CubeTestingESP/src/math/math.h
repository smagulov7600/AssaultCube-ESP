#pragma once
#include <numbers>
#include <cmath>
#include <Windows.h>

#include "../../ext/SimpleMath.h"

using namespace DirectX::SimpleMath;

class Vec2 {
public:
	constexpr Vec2(
		const float x = 0.f,
		const float y = 0.f) noexcept :
		x(x), y(y) { }

	float x, y;
};

class Vec3
{
public:
	constexpr Vec3(
		const float x = 0.f,
		const float y = 0.f,
		const float z = 0.f) noexcept :
		x(x), y(y), z(z) { }

	constexpr const Vec3& operator-(const Vec3& other) const noexcept;
	constexpr const Vec3& operator+(const Vec3& other) const noexcept;
	constexpr const Vec3& operator/(const float factor) const noexcept;
	constexpr const Vec3& operator*(const float factor) const noexcept;

	float x, y, z;
};

static const bool world_to_screen(const Vec3 pos, Vec2* screen, const DirectX::SimpleMath::Matrix& v, int width, int height)
{
	if (width == 0) {
		width = GetSystemMetrics(0);
	}
	if (height == 0) {
		height = GetSystemMetrics(1);
	}

	// No need for transpose unless it's expected in your math
	//Matrix v = view_matrix.Transpose();

	Vector4 clipCoords;
	clipCoords.x = pos.x * v._11 + pos.y * v._21 + pos.z * v._31 + v._41;
	clipCoords.y = pos.x * v._12 + pos.y * v._22 + pos.z * v._32 + v._42;
	clipCoords.z = pos.x * v._13 + pos.y * v._23 + pos.z * v._33 + v._43;
	clipCoords.w = pos.x * v._14 + pos.y * v._24 + pos.z * v._34 + v._44;

	// If the point is too close or behind the camera, don't render
	if (clipCoords.w < 0.1f)
		return false;

	float ndc_x = clipCoords.x / clipCoords.w;
	float ndc_y = clipCoords.y / clipCoords.w;
	float ndc_z = clipCoords.z / clipCoords.w;

	// Screen mapping
	screen->x = (width / 2 * ndc_x) + (ndc_x + width / 2);
	screen->y = (ndc_y + height / 2) - (height / 2 * ndc_y);

	return true;
};