#include "SpriteManager.h"

#include <vector>
#include <unordered_map>

#include <DirectXMath.h>

#include "UploadBuffer.h"
#include "../Utility/D12Helper.h"
#include "../Geometry/Mesh.h"

#define IMPL (*m_impl)
using Microsoft::WRL::ComPtr;
using namespace DirectX;

class SpriteManager::Impl {
	friend SpriteManager;
private:
	Impl();
	~Impl();
private:
	struct Vertex
	{
		XMFLOAT3 Position;
		XMFLOAT2 Size;
	};

	struct ObjectConstant
	{
		XMFLOAT4X4 World;
	};

	struct Loader {
		XMFLOAT2 size;
		ComPtr<ID3D12Resource> Texture;
	};
private:
	bool Has(const std::string& name);
	bool Init(ID3D12GraphicsCommandList* pCmdList);
private:
	ComPtr<ID3D12Device> m_device;

	ComPtr<ID3D12DescriptorHeap> m_objectHeap;
	ComPtr<ID3D12Resource> m_shadowDepthBuffer;
	ComPtr<ID3D12Resource> m_viewDepthBuffer;

	D3D12_GPU_VIRTUAL_ADDRESS m_worldPassAdress = 0;

	Mesh<Vertex> m_mesh;
	UploadBuffer<ObjectConstant> m_objectConstant;
private:
	std::unordered_map<std::string, Loader> m_loaders;

	std::unordered_map<std::string, uint16_t> m_drawArgs;
};

SpriteManager::Impl::Impl()
{

}

SpriteManager::Impl::~Impl()
{

}

bool SpriteManager::Impl::Has(const std::string& name)
{
	return m_loaders.count(name);
}

bool SpriteManager::Impl::Init(ID3D12GraphicsCommandList* pCmdList)
{
	if (m_worldPassAdress == 0) return false;

	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	for (const auto& loader : m_loaders)
	{
		const auto& name = loader.first;
		const auto& data = loader.second;

		m_mesh.DrawArgs[name].StartIndexLocation = indexCount;
		m_mesh.DrawArgs[name].BaseVertexLocation = vertexCount;
		m_mesh.DrawArgs[name].IndexCount = 1;
		++indexCount;
		++vertexCount;
	}

	m_mesh.Indices16.reserve(indexCount);
	m_mesh.Vertices.reserve(vertexCount);

	XMFLOAT3 position = { 0.0f,0.0f,0.0f };
	for (const auto& loader : m_loaders)
	{
		const auto& name = loader.first;
		const auto& data = loader.second;

		m_mesh.Vertices.push_back({ position, data.size });
		m_mesh.Indices16.push_back(0);
	}

	uint16_t index = -1;
	for (const auto& loader : m_loaders)
	{
		const auto& name = loader.first;
		m_drawArgs[name] = ++index;
	}
	
	// each sprite has 1 CBV(objectconstant) and 1 SRV(texture)
	const auto num_descriptors = m_loaders.size() * 2;
	D12Helper::CreateDescriptorHeap(m_device.Get(), m_objectHeap, num_descriptors, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_objectConstant.Create(m_device.Get(), m_loaders.size(), true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_objectHeap->GetCPUDescriptorHandleForHeapStart());
	const auto heap_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = m_objectConstant.ElementSize();
	XMFLOAT4X4 IdentityMatrix;
	XMStoreFloat4x4(&IdentityMatrix, XMMatrixIdentity());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	for (const auto& loader : m_loaders)
	{
		const auto& name = loader.first;
		const auto& data = loader.second;
		const auto& index = m_drawArgs[name];

		cbvDesc.BufferLocation = m_objectConstant.GetGPUVirtualAddress(index);
		auto mappedData = m_objectConstant.GetHandleMappedData(index);

		mappedData->World = IdentityMatrix;

		m_device->CreateConstantBufferView(&cbvDesc, heapHandle);
		heapHandle.Offset(1, heap_size);

		if (data.Texture)
		{
			srvDesc.Format = data.Texture->GetDesc().Format;
			m_device->CreateShaderResourceView(data.Texture.Get(), &srvDesc, heapHandle);
		}
		heapHandle.Offset(1, heap_size);
	}

	m_mesh.CreateBuffers(m_device.Get(), pCmdList);
	m_mesh.CreateViews();
	m_loaders.clear();

	return true;
}

/*
* Public interface method
*/

SpriteManager::SpriteManager():m_impl(new Impl())
{
}

SpriteManager::~SpriteManager()
{
	SAFE_DELETE(m_impl);
}

bool SpriteManager::SetDevice(ID3D12Device* pDevice)
{
	if (!pDevice) return false;
	IMPL.m_device = pDevice;
	return true;
}

bool SpriteManager::SetWorldPassConstantGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS worldPassConstantGpuAddress)
{
	IMPL.m_worldPassAdress = worldPassConstantGpuAddress;
	return true;
}

bool SpriteManager::SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer)
{
	if (!pShadowDepthBuffer) return false;
	IMPL.m_shadowDepthBuffer = pShadowDepthBuffer;
	return true;
}

bool SpriteManager::SetViewDepth(ID3D12Resource* pViewDepthBuffer)
{
	if (!pViewDepthBuffer) return false;
	IMPL.m_viewDepthBuffer = pViewDepthBuffer;
	return true;
}

bool SpriteManager::Create(const std::string& name, float width, float height, ID3D12Resource* pTexture)
{
	if (IMPL.Has(name)) return false;
	IMPL.m_loaders[name].size = { width , height };
	IMPL.m_loaders[name].Texture = pTexture;
	return true;
}

bool SpriteManager::Init(ID3D12GraphicsCommandList* pCmdList)
{
	if (!IMPL.Init(pCmdList)) return false;
	return true;
}

bool SpriteManager::ClearSubresources()
{
	IMPL.m_mesh.ClearSubresource();
	return true;
}

void SpriteManager::Render(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->IASetVertexBuffers(0, 1, &IMPL.m_mesh.VertexBufferView);
	pCmdList->IASetIndexBuffer(&IMPL.m_mesh.IndexBufferView);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);

	pCmdList->SetGraphicsRootConstantBufferView(0, IMPL.m_worldPassAdress);

	pCmdList->SetDescriptorHeaps(1, IMPL.m_objectHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE objectHeapHandle(IMPL.m_objectHeap->GetGPUDescriptorHandleForHeapStart());
	const auto heap_size = IMPL.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	for (const auto& drawArgs : IMPL.m_drawArgs)
	{
		const auto& name = drawArgs.first;
		const auto& index = drawArgs.second;
		const auto& indexCount = IMPL.m_mesh.DrawArgs[name].IndexCount;
		const auto& startIndex = IMPL.m_mesh.DrawArgs[name].StartIndexLocation;
		const auto& baseVertex = IMPL.m_mesh.DrawArgs[name].BaseVertexLocation;

		pCmdList->SetGraphicsRootDescriptorTable(1, objectHeapHandle);
		pCmdList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0);
		objectHeapHandle.Offset(2, heap_size);
	}
}

bool SpriteManager::Move(const std::string& name, float x, float y, float z)
{
	const auto& index = IMPL.m_drawArgs[name];
	auto mappedData = IMPL.m_objectConstant.GetHandleMappedData(index);
	XMStoreFloat4x4(&mappedData->World, XMMatrixTranslation(x, y, z));
	return true;
}

bool SpriteManager::Scale(const std::string& name, float x, float y, float z)
{
	const auto& index = IMPL.m_drawArgs[name];
	auto mappedData = IMPL.m_objectConstant.GetHandleMappedData(index);
	XMStoreFloat4x4(&mappedData->World, XMMatrixScaling(x, y, z));
	return true;
}
