#include "PMDModel.h"
#include <stdio.h>
#include <Windows.h>

bool PMDModel::Load(const char* path)
{
	//Ž¯•ÊŽq"pmd"
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, path, "rb");
	if (fp == nullptr || err != 0)
	{
		char cerr[256];
		strerror_s(cerr, _countof(cerr), err);
		OutputDebugStringA(cerr);
	}
#pragma pack(1)
	struct PMDHeader {
		char id[3];
		// padding
		float version;
		char name[20];
		char comment[256];
	};

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal_vec;
		DirectX::XMFLOAT2 uv;
		WORD bone_num[2];
		// 36 bytes
		BYTE bone_weight;
		BYTE edge_flag;
		// 38 bytes
		// padding 2 bytes
	};

	struct Material
	{
		DirectX::XMFLOAT3 diffuse;
		FLOAT alpha;
		FLOAT specularity;
		FLOAT specular_color[3];
		FLOAT mirror_color[3];
		BYTE toon_index;
		BYTE edge_flag;
		DWORD face_vert_count;
		char textureFilePath[20];
	};
#pragma pack()

	PMDHeader header;
	fread_s(&header, sizeof(header), sizeof(header), 1, fp);
	uint32_t cVertex = 0;
	fread_s(&cVertex, sizeof(cVertex), sizeof(cVertex), 1, fp);
	std::vector<Vertex> vertices(cVertex);
	fread_s(vertices.data(), vertices.size() * sizeof(Vertex), vertices.size() *  sizeof(Vertex), 1, fp);
	vertices_.resize(cVertex);
	for (int i = 0; i < cVertex; ++i)
	{
		vertices_[i].pos = vertices[i].pos;
		vertices_[i].normal = vertices[i].normal_vec;
		vertices_[i].uv = vertices[i].uv;
	}
	uint32_t cIndex = 0;
	fread_s(&cIndex, sizeof(cIndex), sizeof(cIndex), 1, fp);
	indices_.resize(cIndex);
	fread_s(indices_.data(), sizeof(indices_[0])* indices_.size(), sizeof(indices_[0])* indices_.size(), 1, fp);
	uint32_t cMaterial = 0;
	fread_s(&cMaterial, sizeof(cMaterial), sizeof(cMaterial), 1, fp);
	std::vector<Material> materials(cMaterial);
	fread_s(materials.data(), sizeof(materials[0])* materials.size(), sizeof(materials[0])* materials.size(), 1, fp);
	for (auto& m : materials)
	{
		materials_.push_back({ m.diffuse,m.face_vert_count });
	}

	fclose(fp);

	return true;
}

const std::vector<Vertex>& PMDModel::GetVerices() const
{
	return vertices_;
}

const std::vector<uint16_t>& PMDModel::GetIndices() const
{
	return indices_;
}

const std::vector<PMDMaterial>& PMDModel::GetMaterials() const
{
	return materials_;
}
