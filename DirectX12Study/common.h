#pragma once
#include <DirectXMath.h>

struct Size
{
	size_t width;
	size_t height;
};

struct Vertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
};