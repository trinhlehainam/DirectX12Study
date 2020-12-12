#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <DirectXMath.h>
#include <d3d12.h>
#include <d3dx12.h>
#include <memory>

#include "../common.h"
#include "../DirectX12/UploadBuffer.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class VMDMotion;

class PMDModel
{
public:
	bool LoadPMD(const char* path);
	void LoadMotion(const char* path);
	void GetDefaultTexture(ComPtr<ID3D12Resource>& whiteTexture,
		ComPtr<ID3D12Resource>& blackTexture,
		ComPtr<ID3D12Resource>& gradTexture);
	void CreateModel();
	void Transform(const DirectX::XMMATRIX& transformMatrix);

	uint16_t GetIndicesSize() const;

	void SetInputAssembler(ComPtr<ID3D12GraphicsCommandList>& cmdList);

	void SetTransformDescriptorTable(ComPtr<ID3D12GraphicsCommandList>& cmdList);

	void Render(ComPtr<ID3D12GraphicsCommandList>& cmdList, const size_t& frame);

	PMDModel() = default;
	PMDModel(ComPtr<ID3D12Device> device);
	~PMDModel();
private:
	friend class PMDManager;

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
		uint32_t indices;
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

	std::vector<PMDVertex> vertices_;
	std::vector<uint16_t> indices_;
	std::vector<PMDMaterial> materials_;
	std::vector<std::string> modelPaths_;
	std::vector<std::string> toonPaths_;
	std::vector<PMDBone> m_boneMatrices;
	std::unordered_map<std::string, uint16_t> bonesTable_;
	std::vector<DirectX::XMMATRIX> boneMatrices_;
	const char* pmdPath_;

private:
	ComPtr<ID3D12Device> m_device = nullptr;

	ComPtr<ID3D12Resource> whiteTexture_;
	ComPtr<ID3D12Resource> blackTexture_;
	ComPtr<ID3D12Resource> gradTexture_;

	struct ObjectConstant
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX bones[512];
	};

	// Vertex Buffer
	ComPtr<ID3D12Resource> vertexBuffer_;				//頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	PMDVertex* mappedVertex_ = nullptr;
	// 頂点バッファを生成 (して、CPU側の頂点情報をコピー)
	void CreateVertexBuffer();

	ComPtr<ID3D12Resource> indicesBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;
	void CreateIndexBuffer();

	// Object Constant
	UploadBuffer<ObjectConstant> m_transformBuffer;
	ComPtr<ID3D12DescriptorHeap> m_transformDescHeap;
	bool CreateTransformConstant();

	// Bone buffer
	ComPtr<ID3D12Resource> boneBuffer_;
	DirectX::XMMATRIX* mappedBoneMatrix_ = nullptr;
	bool CreateBoneBuffer();

	std::vector<ComPtr<ID3D12Resource>> textureBuffers_;
	std::vector<ComPtr<ID3D12Resource>> sphBuffers_;
	std::vector<ComPtr<ID3D12Resource>> spaBuffers_;
	std::vector<ComPtr<ID3D12Resource>> toonBuffers_;
	// Create texture from PMD file
	void LoadTextureToBuffer();
	ComPtr<ID3D12Resource> materialBuffer_;
	ComPtr<ID3D12DescriptorHeap> materialDescHeap_;
	bool CreateMaterialAndTextureBuffer();

	bool CreateTexture(void);

private:

	std::shared_ptr<VMDMotion> vmdMotion_;

	void UpdateMotionTransform(const size_t& keyframe = 0);
	void RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& mats, size_t index);
	// Root-finding algorithm ( finding ZERO or finding ROOT )
	// There are 4 beizer points, but 2 of them are default at (0,0) and (127, 127)->(1,1) respectively
	float CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n = 8);
};

