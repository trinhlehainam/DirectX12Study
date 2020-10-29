#pragma once

#include <vector>
#include <cstdint>
#include "../common.h"

struct PMDMaterial
{
	DirectX::XMFLOAT3 diffuse; // diffuse color;
	uint32_t indices;
};

class PMDModel
{
private:
	std::vector<Vertex> vertices_;
	std::vector<uint16_t> indices_;
	std::vector<PMDMaterial> materials_;
public:
	bool Load(const char* path);
	const std::vector<Vertex>& GetVerices() const;
	const std::vector<uint16_t>& GetIndices() const;
	const std::vector<PMDMaterial>& GetMaterials() const;
};

