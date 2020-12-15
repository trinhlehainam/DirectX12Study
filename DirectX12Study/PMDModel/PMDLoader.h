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
private:
	friend class PMDModel;

	std::vector<PMDVertex> m_vertices;
	std::vector<uint16_t> m_indices;
	std::vector<PMDMaterial> m_materials;
	std::vector<PMDSubMaterial> m_subMaterials;
	std::vector<std::string> m_modelPaths;
	std::vector<std::string> m_toonPaths;
	std::vector<PMDBone> m_bones;
	std::unordered_map<std::string, uint16_t> m_bonesTable;
	const char* m_path;
};

