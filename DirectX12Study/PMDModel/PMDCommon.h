#pragma once
#include <DirectXMath.h>

struct PMDVertex
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 uv;
	uint16_t boneNo[2];
	float weight;
};