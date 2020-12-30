#include "PMDModel.h"
#include "../Application.h"
#include "VMD/VMDMotion.h"
#include "../Utility/D12Helper.h"
#include "../Utility/StringHelper.h"
#include "PMDLoader.h"

#include <stdio.h>
#include <Windows.h>
#include <array>
#include <d3dcompiler.h>

namespace
{
	const char* pmd_path = "resource/PMD/";
	constexpr unsigned int material_descriptor_count_per_heap = 5;
}

PMDModel::PMDModel()
{
	m_pmdLoader = std::make_unique<PMDLoader>();
}

PMDModel::PMDModel(ID3D12Device* device) :
	m_device(device)
{
	m_pmdLoader = std::make_unique<PMDLoader>();
}

PMDModel::~PMDModel()
{
	m_device = nullptr;
	m_whiteTexture = nullptr;
	m_blackTexture = nullptr;
	m_gradTexture = nullptr;
}

void PMDModel::CreateModel(ID3D12GraphicsCommandList* cmdList)
{
	CreateTransformConstantBuffer();
	LoadTextureToBuffer();
	CreateMaterialAndTextureBuffer(cmdList);
}

const std::vector<uint16_t>& PMDModel::Indices() const
{
	return m_pmdLoader->Indices;
}

const std::vector<PMDVertex>& PMDModel::Vertices() const
{
	return m_pmdLoader->Vertices;
}

void PMDModel::ClearSubresources()
{
	m_pmdLoader.reset();
}

void PMDModel::SetDefaultTexture(ID3D12Resource* whiteTexture, 
	ID3D12Resource* blackTexture, ID3D12Resource* gradTexture)
{
	m_whiteTexture = whiteTexture;
	m_blackTexture = blackTexture;
	m_gradTexture = gradTexture;
}

void PMDModel::SetDevice(ID3D12Device* pDevice)
{
	m_device = pDevice;
}

bool PMDModel::Load(const char* path)
{
	m_pmdLoader->Load(path);
	RenderResource.SubMaterials = std::move(m_pmdLoader->SubMaterials);
	Bones = std::move(m_pmdLoader->Bones);
	BonesTable = std::move(m_pmdLoader->BonesTable);
	return true;
}

bool PMDModel::CreateTransformConstantBuffer()
{
	Resource.TransformConstant.Create(m_device, 1, true);

	auto mappedData = Resource.TransformConstant.HandleMappedData();
	mappedData->world = XMMatrixIdentity();
	std::vector<XMMATRIX> defaultMatrices;
	defaultMatrices.reserve(512);
	for (uint16_t i = 0; i < 512; ++i)
	{
		defaultMatrices.push_back(XMMatrixIdentity());
	}
	auto bones = defaultMatrices;

	std::copy(bones.begin(), bones.end(), mappedData->bones);

	return true;
}

bool PMDModel::CreateMaterialAndTextureBuffer(ID3D12GraphicsCommandList* cmdList)
{
	HRESULT result = S_OK;
	const size_t num_materials_block = m_pmdLoader->Materials.size();

	D12Helper::CreateDescriptorHeap(m_device, RenderResource.MaterialHeap, num_materials_block * material_descriptor_count_per_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart());
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//
	// Map materials data to materials default buffer
	//
	size_t strideBytes = D12Helper::AlignedConstantBufferMemory(sizeof(PMDMaterial));
	size_t sizeInBytes = num_materials_block * strideBytes;

	Resource.MaterialConstant = D12Helper::CreateBuffer(m_device, sizeInBytes);
	auto gpuAddress = Resource.MaterialConstant->GetGPUVirtualAddress();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = strideBytes;
	cbvDesc.BufferLocation = gpuAddress;

	// Material constant
	uint8_t* m_mappedMaterial = nullptr;
	result = Resource.MaterialConstant->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedMaterial));
	assert(SUCCEEDED(result));
	auto& material = m_pmdLoader->Materials;
	// index of material on each block
	size_t index = 0;
	for (int i = 0; i < num_materials_block; ++i)
	{
		auto mappedData = reinterpret_cast<PMDMaterial*>(m_mappedMaterial);
		(*mappedData) = material[i];
		cbvDesc.BufferLocation = gpuAddress;
		m_device->CreateConstantBufferView(
			&cbvDesc,
			heapAddress);
		// move memory offset
		m_mappedMaterial += strideBytes;
		gpuAddress += strideBytes;
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	Resource.MaterialConstant->Unmap(0, nullptr);
	++index;

	// Material texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ¦

	// Texture
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for main texture (image)
		srvDesc.Format = Resource.Textures[i] ? Resource.Textures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.Textures[i].Get() ? m_whiteTexture : Resource.Textures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	++index;
	// sph
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for sphere mapping texture (sph)
		srvDesc.Format = Resource.sphTextures[i] ? Resource.sphTextures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.sphTextures[i].Get() ? m_whiteTexture : Resource.sphTextures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	++index;
	// spa
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for sphere mapping texture (spa)
		srvDesc.Format = Resource.spaTextures[i] ? Resource.spaTextures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.spaTextures[i].Get() ? m_blackTexture : Resource.spaTextures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	++index;
	// toon
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for toon map
		srvDesc.Format = Resource.ToonTextures[i] ? Resource.ToonTextures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.ToonTextures[i].Get() ? m_gradTexture : Resource.ToonTextures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}

	return true;
}

void PMDModel::LoadTextureToBuffer()
{
	Resource.Textures.resize(m_pmdLoader->ModelPaths.size());
	Resource.sphTextures.resize(m_pmdLoader->ModelPaths.size());
	Resource.spaTextures.resize(m_pmdLoader->ModelPaths.size());
	Resource.ToonTextures.resize(m_pmdLoader->ToonPaths.size());

	auto& Textures = Resource.Textures;
	auto& sphTextures = Resource.sphTextures;
	auto& spaTextures = Resource.spaTextures;
	auto& ToonTextures = Resource.ToonTextures;

	for (int i = 0; i < Textures.size(); ++i)
	{
		if (!m_pmdLoader->ToonPaths[i].empty())
		{
			std::string toonPath;
			
			toonPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, m_pmdLoader->ToonPaths[i].c_str());
			ToonTextures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			if (!ToonTextures[i])
			{
				toonPath = std::string(pmd_path) + "toon/" + m_pmdLoader->ToonPaths[i];
				ToonTextures[i]= D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			}
		}
		if (!m_pmdLoader->ToonPaths[i].empty())
		{
			auto splittedPaths = StringHelper::SplitFilePath(m_pmdLoader->ModelPaths[i]);
			for (auto& path : splittedPaths)
			{
				auto ext = StringHelper::GetFileExtension(path);
				if (ext == "sph")
				{
					auto sphPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, path.c_str());
					sphTextures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(sphPath));
				}
				else if (ext == "spa")
				{
					auto spaPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, path.c_str());
					spaTextures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(spaPath));
				}
				else
				{
					auto texPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, path.c_str());
					Textures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(texPath));
				}
			}
		}
	}

}
