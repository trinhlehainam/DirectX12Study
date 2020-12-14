#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

#include <DirectXMath.h>
#include <d3dx12.h>

#include "../common.h"
#include "../Utility/UploadBuffer.h"
#include "PMDCommon.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class VMDMotion;
class PMDLoader;

class PMDModel
{
public:
	PMDModel();
	PMDModel(ComPtr<ID3D12Device> pDevice);
	~PMDModel();

	bool LoadPMD(const char* path);
	void LoadMotion(const char* path);
	void SetDefaultTexture(ID3D12Resource* whiteTexture,
						   ID3D12Resource* blackTexture,
						   ID3D12Resource* gradTexture);

	void SetDevice(ID3D12Device* pDevice);

	void CreateModel();
	void Transform(const DirectX::XMMATRIX& transformMatrix);

	void Render(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
		const uint32_t& BaseVertexLocation);
	void RenderDepth(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
		const uint32_t& BaseVertexLocation);
	
	const std::vector<uint16_t>& Indices() const;
	const std::vector<PMDVertex>& Vertices() const;
private:
	std::unique_ptr<PMDLoader> m_pmdLoader;

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
	DirectX::XMMATRIX* m_mappedBoneMatrix = nullptr;
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

	std::shared_ptr<VMDMotion> m_vmdMotion;

	void UpdateMotionTransform(const size_t& keyframe = 0);
	void RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& mats, size_t index);
	// Root-finding algorithm ( finding ZERO or finding ROOT )
	// There are 4 beizer points, but 2 of them are default at (0,0) and (127, 127)->(1,1) respectively
	float CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n = 8);
};

