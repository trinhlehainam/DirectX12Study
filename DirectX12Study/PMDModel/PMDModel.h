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
#include "../Utility/UploadBuffer.h"
#include "PMDCommon.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class VMDMotion;

class PMDModel
{
public:
	bool LoadPMD(const char* path);
	void LoadMotion(const char* path);
	void GetDefaultTexture(ID3D12Resource* whiteTexture,
						   ID3D12Resource* blackTexture,
						   ID3D12Resource* gradTexture);
	void CreateModel();
	void Transform(const DirectX::XMMATRIX& transformMatrix);

	void Render(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
		const uint32_t& BaseVertexLocation);

	void RenderDepth(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
		const uint32_t& BaseVertexLocation);
	
	PMDModel() = default;
	PMDModel(ComPtr<ID3D12Device> device);
	~PMDModel();
private:
	friend class PMDManager;

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
	std::vector<PMDBone> m_bones;
	std::unordered_map<std::string, uint16_t> bonesTable_;
	std::vector<DirectX::XMMATRIX> boneMatrices_;
	const char* pmdPath_;

private:
	ComPtr<ID3D12Device> m_device = nullptr;

	ID3D12Resource* m_whiteTexture;
	ID3D12Resource* m_blackTexture;
	ID3D12Resource* m_gradTexture;

	struct ObjectConstant
	{
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX bones[512];
	};

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

private:

	std::shared_ptr<VMDMotion> vmdMotion_;

	void UpdateMotionTransform(const size_t& keyframe = 0);
	void RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& mats, size_t index);
	// Root-finding algorithm ( finding ZERO or finding ROOT )
	// There are 4 beizer points, but 2 of them are default at (0,0) and (127, 127)->(1,1) respectively
	float CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n = 8);
};

