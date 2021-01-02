#pragma once
#include <DirectXMath.h>

struct Material
{
	Material() = default;
	explicit Material(DirectX::XMFLOAT4 DiffuseAlbedo, DirectX::XMFLOAT3 FresnelF0, float Roughness);

	DirectX::XMFLOAT4 DiffuseAlbedo;
	DirectX::XMFLOAT3 FresnelF0;
	float Roughness;
};


