#pragma once
#include <vector>
#include <unordered_map>
#include <string>

struct PMDSubMesh
{
	uint16_t baseIndex;
};

struct PMDMeshes
{
	std::vector<uint16_t> indices;
	std::unordered_map<std::string, PMDSubMesh> meshIndex;
};

