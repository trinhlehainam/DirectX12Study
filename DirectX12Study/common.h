#pragma once
#include <DirectXMath.h>

struct Size
{
	size_t width;
	size_t height;
};

constexpr float second_to_millisecond = 1000.0f;
constexpr float FPS = 60.0f;
constexpr float millisecond_per_frame = second_to_millisecond / FPS;
