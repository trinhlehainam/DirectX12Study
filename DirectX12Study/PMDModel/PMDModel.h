#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>

#include "../DirectX12/UploadBuffer.h"
#include "PMDCommon.h"

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class VMDMotion;
class PMDLoader;

struct PMDResource
{
	std::vector<ComPtr<ID3D12Resource>> Textures;
	std::vector<ComPtr<ID3D12Resource>> sphTextures;
	std::vector<ComPtr<ID3D12Resource>> spaTextures;
	std::vector<ComPtr<ID3D12Resource>> ToonTextures;
	ComPtr<ID3D12Resource> MaterialConstant;
	UploadBuffer<PMDObjectConstant> TransformConstant;
	void operator = (PMDResource&& other) noexcept
	{
		Textures = std::move(other.Textures);
		sphTextures = std::move(other.sphTextures);
		spaTextures = std::move(other.spaTextures);
		ToonTextures = std::move(other.ToonTextures);
		MaterialConstant = other.MaterialConstant;
		TransformConstant.Move(other.TransformConstant);

		other.MaterialConstant = nullptr;
	}
};

struct PMDRenderResource
{
	std::vector<PMDSubMaterial> SubMaterials;
	ComPtr<ID3D12DescriptorHeap> TransformHeap;
	ComPtr<ID3D12DescriptorHeap> MaterialHeap;
	void operator = (PMDRenderResource&& other) noexcept
	{
		SubMaterials = std::move(other.SubMaterials);
		TransformHeap = other.TransformHeap;
		MaterialHeap = other.MaterialHeap;

		other.TransformHeap = nullptr;
		other.MaterialHeap = nullptr;
	}
};

class PMDModel
{
public:
	
public:
	PMDModel();
	explicit PMDModel(ID3D12Device* pDevice);
	~PMDModel();

	void SetDevice(ID3D12Device* pDevice);
	bool Load(const char* path);
	void SetDefaultTexture(ID3D12Resource* whiteTexture,
						   ID3D12Resource* blackTexture,
						   ID3D12Resource* gradTexture);
	void CreateModel(ID3D12GraphicsCommandList* cmdList);

	const std::vector<uint16_t>& Indices() const;
	const std::vector<PMDVertex>& Vertices() const;

	void ClearSubresources();
public:
	void Play(VMDMotion* animation);
	void Move(const float& moveX, const float& moveY, const float& moveZ);
	void RotateX(const float& angle);
	void RotateY(const float& angle);
	void RotateZ(const float& angle);
	void Scale(const float& scaleX, const float& scaleY, const float& scaleZ);

	void Update(const float& deltaTime);
	
	PMDResource Resource;
	PMDRenderResource RenderResource;
private:
	// Resource from PMD Manager
	ID3D12Device* m_device = nullptr;
	ID3D12Resource* m_whiteTexture = nullptr;
	ID3D12Resource* m_blackTexture = nullptr;
	ID3D12Resource* m_gradTexture = nullptr;

	std::unique_ptr<PMDLoader> m_pmdLoader;
private:
	// Vairables for animation
	std::vector<PMDBone> m_bones;
	std::unordered_map<std::string, uint16_t> m_bonesTable;
	std::vector<DirectX::XMMATRIX> m_boneMatrices;
	VMDMotion* m_vmdMotion = nullptr;
	float m_timer = 0.0f;
	uint64_t m_frameCnt = 0.0f;

private:
	bool CreateTransformConstantBuffer();
	// Create texture from PMD file
	void LoadTextureToBuffer();
	bool CreateMaterialAndTextureBuffer(ID3D12GraphicsCommandList* cmdList);
private:
	void Transform(const DirectX::XMMATRIX& transformMatrix);

	void UpdateMotionTransform(const size_t& keyframe = 0);
	void RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& mats, size_t index);
	// Root-finding algorithm ( finding ZERO or finding ROOT )
	// There are 4 beizer points, but 2 of them are default at (0,0) and (127, 127)->(1,1) respectively
	float CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n = 8);
};

