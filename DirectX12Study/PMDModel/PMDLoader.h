#pragma once

#include <vector>
#include <unordered_map>

#include "PMDCommon.h"

class PMDLoader
{
public:
	PMDLoader() = default;
	~PMDLoader() = default;
	bool Load(const char* path);

	std::vector<PMDVertex> Vertices;
	std::vector<uint16_t> Indices;
	std::vector<PMDMaterial> Materials;
	std::vector<PMDSubMaterial> SubMaterials;
	std::vector<std::string> ModelPaths;
	std::vector<std::string> ToonPaths;
	std::vector<PMDBone> Bones;
	std::unordered_map<std::string, uint16_t> BoneTable;
	const char* Path;
};

