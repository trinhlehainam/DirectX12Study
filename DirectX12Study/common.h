#pragma once
#include <DirectXMath.h>

struct Size
{
	size_t width;
	size_t height;
};

struct BasicMatrix {
	DirectX::XMMATRIX world;
	DirectX::XMMATRIX viewproj;
	DirectX::XMVECTOR lightPos;
	DirectX::XMMATRIX shadow;
};

struct BasicMaterial {
	DirectX::XMFLOAT3 diffuse;
	float alpha;
	DirectX::XMFLOAT3 specular;
	float specularity;
	DirectX::XMFLOAT3 ambient;
};

constexpr float second_to_millisecond = 1000.0f;
constexpr float FPS = 60.f;
constexpr float millisecond_per_frame = second_to_millisecond / FPS;

