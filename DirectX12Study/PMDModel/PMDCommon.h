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

struct PMDMaterial
{
	DirectX::XMFLOAT3 diffuse; // diffuse color;
	float alpha;
	DirectX::XMFLOAT3 specular;
	float specularity;
	DirectX::XMFLOAT3 ambient;
};

struct PMDSubMaterial
{
	uint32_t indexCount;
};

struct PMDBone
{
	std::string name;
	std::vector<int> children;
#ifdef _DEBUG
	std::vector<std::string> childrenName;
#endif
	DirectX::XMFLOAT3 pos;			// rotation at origin position
};