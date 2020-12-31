#pragma once

#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <memory>
#include <d3dx12.h>

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

	PMDResource() = default;
	explicit PMDResource(PMDResource&& other) noexcept
		:Textures(std::move(other.Textures)),
		sphTextures(std::move(other.sphTextures)),
		spaTextures(std::move(other.spaTextures)),
		ToonTextures(std::move(other.ToonTextures)),
		MaterialConstant(other.MaterialConstant)
	{
		other.MaterialConstant = nullptr;
	}
	void operator = (PMDResource&& other) noexcept
	{
		Textures = std::move(other.Textures);
		sphTextures = std::move(other.sphTextures);
		spaTextures = std::move(other.spaTextures);
		ToonTextures = std::move(other.ToonTextures);
		MaterialConstant = other.MaterialConstant;

		other.MaterialConstant = nullptr;
	}
};

struct PMDRenderResource
{
	std::vector<PMDSubMaterial> SubMaterials;
	uint16_t MaterialsHeapOffset = 0;
	PMDRenderResource() = default;
	explicit PMDRenderResource(PMDRenderResource&& other) noexcept 
		:SubMaterials(std::move(other.SubMaterials)),
		MaterialsHeapOffset(other.MaterialsHeapOffset)
	{
		other.MaterialsHeapOffset = 0;
	}
	void operator = (PMDRenderResource&& other) noexcept
	{
		SubMaterials = std::move(other.SubMaterials);
		MaterialsHeapOffset = other.MaterialsHeapOffset;

		other.MaterialsHeapOffset = 0;
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
	void CreateModel(ID3D12GraphicsCommandList* cmdList, CD3DX12_CPU_DESCRIPTOR_HANDLE& heapHandle);

	const std::vector<uint16_t>& Indices() const;
	const std::vector<PMDVertex>& Vertices() const;

	void ClearSubresources();
public:
	
	PMDResource Resource;
	PMDRenderResource RenderResource;
	std::vector<PMDBone> Bones;
	std::unordered_map<std::string, uint16_t> BonesTable;
	uint16_t MaterialDescriptorCount = 0;
private:
	// Resource from PMD Manager
	ID3D12Device* m_device = nullptr;
	ID3D12Resource* m_whiteTexture = nullptr;
	ID3D12Resource* m_blackTexture = nullptr;
	ID3D12Resource* m_gradTexture = nullptr;

	std::unique_ptr<PMDLoader> m_pmdLoader;

private:
	// Create texture from PMD file
	void LoadTextureToBuffer();
	bool CreateMaterialAndTextureBuffer(ID3D12GraphicsCommandList* cmdList, CD3DX12_CPU_DESCRIPTOR_HANDLE& heapHandle);
};

