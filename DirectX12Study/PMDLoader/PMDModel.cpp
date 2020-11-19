#include "PMDModel.h"
#include <stdio.h>
#include <Windows.h>
#include <array>

namespace
{
	std::string GetTexturePathFromModelPath(const char* modelPath, const char* texturePath)
	{
		std::string ret = modelPath;
		auto idx = ret.rfind("/") + 1;

		return ret.substr(0, idx) + texturePath;
	}
}

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
		DirectX::XMFLOAT3 specular_color;
		DirectX::XMFLOAT3 mirror_color;
		BYTE toon_index;
		BYTE edge_flag;
		DWORD face_vert_count;
		char textureFileName[20];
	};

	struct BoneData
	{
		char boneName[20];
		uint16_t parentNo;
		uint16_t tailNo;
		uint8_t type;
		uint16_t ikParentNo;
		DirectX::XMFLOAT3 pos;
	}; // 39 bytes
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
		std::copy(std::begin(vertices[i].bone_num),
			std::end(vertices[i].bone_num), vertices_[i].boneNo);
		vertices_[i].weight = static_cast<float>(vertices[i].bone_weight) / 100.0f;
	}
	uint32_t cIndex = 0;
	fread_s(&cIndex, sizeof(cIndex), sizeof(cIndex), 1, fp);
	indices_.resize(cIndex);
	fread_s(indices_.data(), sizeof(indices_[0])* indices_.size(), sizeof(indices_[0])* indices_.size(), 1, fp);
	uint32_t cMaterial = 0;
	fread_s(&cMaterial, sizeof(cMaterial), sizeof(cMaterial), 1, fp);
	std::vector<Material> materials(cMaterial);
	fread_s(materials.data(), sizeof(materials[0])* materials.size(), sizeof(materials[0])* materials.size(), 1, fp);

	uint16_t boneNum = 0;
	fread_s(&boneNum, sizeof(boneNum), sizeof(boneNum), 1, fp);

	// Bone
	std::vector<BoneData> boneData(boneNum);
	fread_s(boneData.data(), sizeof(boneData[0])* boneData.size(), sizeof(boneData[0])* boneData.size(), 1, fp);
	bones_.resize(boneNum);
	for (int i = 0; i < boneNum; ++i)
	{
		bones_[i].name = boneData[i].boneName;
		bones_[i].pos = boneData[i].pos;
		bonesTable_[boneData[i].boneName] = i;
	}
	boneMatrices_.resize(boneNum);
	std::fill(boneMatrices_.begin(), boneMatrices_.end(), DirectX::XMMatrixIdentity());

	for (int i = 0; i < boneNum; ++i)
	{
		if (boneData[i].parentNo == 0xffff) continue;
		auto pno = boneData[i].parentNo;
		bones_[pno].children.push_back(i);
#ifdef _DEBUG
		bones_[pno].childrenName.push_back(boneData[i].boneName);
#endif
	}

	// IK(inverse kematic)
	uint16_t ikNum = 0;
	fread_s(&ikNum, sizeof(ikNum), sizeof(ikNum), 1, fp);
	for (int i = 0; i < ikNum; ++i)
	{
		fseek(fp, 4, SEEK_CUR);
		uint8_t chainNum;
		fread_s(&chainNum, sizeof(chainNum), sizeof(chainNum), 1, fp);
		fseek(fp, sizeof(uint16_t) + sizeof(float), SEEK_CUR);
		fseek(fp, sizeof(uint16_t) * chainNum, SEEK_CUR);
	}

	uint16_t skinNum = 0;
	fread_s(&skinNum, sizeof(skinNum), sizeof(skinNum), 1, fp);
	for (int i = 0; i < skinNum; ++i)
	{
		fseek(fp, 20, SEEK_CUR);	// Name of facial skin
		uint32_t skinVertCnt = 0;		// number of facial vertex
		fread_s(&skinVertCnt, sizeof(skinVertCnt), sizeof(skinVertCnt), 1, fp);
		fseek(fp, 1, SEEK_CUR);		// kind of facial
		fseek(fp, 16 * skinVertCnt, SEEK_CUR); // position of vertices
	}

	uint8_t skinDispNum = 0;
	fread_s(&skinDispNum, sizeof(skinDispNum), sizeof(skinDispNum), 1, fp);
	fseek(fp, sizeof(uint16_t)* skinDispNum, SEEK_CUR);

	uint8_t ikNameNum = 0;
	fread_s(&ikNameNum, sizeof(ikNameNum), sizeof(ikNameNum), 1, fp);
	fseek(fp, 50 * ikNameNum, SEEK_CUR);

	uint32_t boneDispNum = 0;
	fread_s(&boneDispNum, sizeof(boneDispNum), sizeof(boneDispNum), 1, fp);
	fseek(fp, 
		(sizeof(uint16_t) + sizeof(uint8_t)) * boneDispNum, 
		SEEK_CUR);

	uint8_t isEngAvalable = 0;
	fread_s(&isEngAvalable, sizeof(isEngAvalable), sizeof(isEngAvalable), 1, fp);
	if (isEngAvalable)
	{
		fseek(fp, 276, SEEK_CUR);
		fseek(fp, boneNum * 20, SEEK_CUR);
		fseek(fp, (skinNum-1) * 20, SEEK_CUR);		// list of facial skin's English name
		fseek(fp, ikNameNum * 50, SEEK_CUR);
	}
	
	std::array<char[100], 10> toonNames;
	fread_s(toonNames.data(), sizeof(toonNames[0])* toonNames.size(), sizeof(toonNames[0])* toonNames.size(), 1, fp);

	for (auto& m : materials)
	{
		if(m.toon_index > toonNames.size() - 1)
			toonPaths_.push_back(toonNames[0]);
		else
			toonPaths_.push_back(toonNames[m.toon_index]);

		modelPaths_.push_back(m.textureFileName);
		materials_.push_back({ m.diffuse,m.alpha,m.specular_color,m.specularity,m.mirror_color,m.face_vert_count });
	}

	fclose(fp);

	return true;
}

const std::vector<PMDVertex>& PMDModel::GetVerices() const
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

const std::vector<std::string>& PMDModel::GetModelPaths() const
{
	return modelPaths_;
}

const std::vector<std::string>& PMDModel::GetToonPaths() const
{
	return toonPaths_;
}

const std::vector<PMDBone>& PMDModel::GetBoneData() const
{
	return bones_;
}

const std::unordered_map<std::string, uint16_t>& PMDModel::GetBonesTable() const
{
	return bonesTable_;
}

const std::vector<DirectX::XMMATRIX>& PMDModel::GetBoneMatrices() const
{
	return boneMatrices_;
}
