#pragma once
#include <DirectXMath.h>

struct Size
{
	size_t width;
	size_t height;
};

struct WorldPassConstant {
	DirectX::XMFLOAT4X4 viewProj;
	DirectX::XMFLOAT4X4 lightViewProj;
	DirectX::XMFLOAT3 viewPos;
	float padding0;
	DirectX::XMFLOAT3 lightDir;
	float padding1;
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

