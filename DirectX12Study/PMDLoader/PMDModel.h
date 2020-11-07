#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include "../common.h"

struct PMDMaterial
{
	DirectX::XMFLOAT3 diffuse; // diffuse color;
	float alpha;
	DirectX::XMFLOAT3 specular;
	float specularity;
	DirectX::XMFLOAT3 ambient;
	uint32_t indices;
};

class PMDModel
{
private:
	std::vector<Vertex> vertices_;
	std::vector<uint16_t> indices_;
	std::vector<PMDMaterial> materials_;
	std::vector<std::string> modelPaths_;
	std::vector<std::string> toonPaths_;
public:
	bool Load(const char* path);
	const std::vector<Vertex>& GetVerices() const;
	const std::vector<uint16_t>& GetIndices() const;
	const std::vector<PMDMaterial>& GetMaterials() const;
	const std::vector<std::string>& GetModelPaths() const;
	const std::vector<std::string>& GetToonPaths() const;
};

