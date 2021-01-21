#include "PrimitiveManager.h"

#include <cassert>
#include <unordered_map>

#include "Mesh.h"
#include "../Utility/D12Helper.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/UploadBuffer.h"

#define IMPL (*m_impl)

using Microsoft::WRL::ComPtr;
using namespace DirectX;

class PrimitiveManager::Impl
{
public:
	Impl();
	Impl(ID3D12Device* pDevice);
	~Impl();
private:
	friend PrimitiveManager;

	bool CreateObjectHeap();
	bool Has(const std::string& name);
private:
	bool m_isInitDone = false;
	// Device from engine
	ComPtr<ID3D12Device> m_device;

	D3D12_GPU_VIRTUAL_ADDRESS m_worldPassAdress = 0;

	// Descriptor heap stores descriptor of depth buffer
	// Use for binding resource of engine to this pipeline
	ComPtr<ID3D12DescriptorHeap> m_depthHeap;
	uint16_t m_depthBufferCount = 0;

	struct ObjectConstant
	{
		XMFLOAT4X4 World;
		XMFLOAT4X4 TexTransform;
	};

	UploadBuffer<ObjectConstant> m_objectConstant;
	ComPtr<ID3D12DescriptorHeap> m_objectHeap;

	ComPtr<ID3D12Resource> m_whiteTex;
	ComPtr<ID3D12Resource> m_blackTex;
	ComPtr<ID3D12Resource> m_gradTex;

private:
	Mesh<Geometry::Vertex> m_mesh;
	struct Loader
	{
		Geometry::Mesh Primitive;
		D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAdress;
		ComPtr<ID3D12Resource> Texture;
	};
	std::unordered_map<std::string, Loader> m_loaders;
	struct DrawData
	{
		uint16_t Index;
		D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAddress;
	};
	std::unordered_map<std::string, DrawData> m_drawDatas;
};

PrimitiveManager::Impl::Impl()
{

}

PrimitiveManager::Impl::Impl(ID3D12Device* pDevice):m_device(pDevice)
{

}

PrimitiveManager::Impl::~Impl()
{
	m_device = nullptr;
}

bool PrimitiveManager::SetDefaultTexture(ID3D12Resource* whiteTexture, ID3D12Resource* blackTexture, ID3D12Resource* gradiationTexture)
{
	if (!whiteTexture) return false;
	if (!blackTexture) return false;
	if (!gradiationTexture) return false;

	IMPL.m_whiteTex = whiteTexture;
	IMPL.m_blackTex = blackTexture;
	IMPL.m_gradTex = gradiationTexture;

	return true;
}

bool PrimitiveManager::Impl::CreateObjectHeap()
{
	const auto num_primitive = m_drawDatas.size() * 2;
	D12Helper::CreateDescriptorHeap(m_device.Get(), m_objectHeap, num_primitive, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_objectConstant.Create(m_device.Get(), num_primitive, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_objectHeap->GetCPUDescriptorHandleForHeapStart());
	const auto heap_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Transform Constant
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = m_objectConstant.ElementSize();
	for (const auto& drawData : m_drawDatas)
	{
		const auto& name = drawData.first;
		const auto& data = drawData.second;
		const auto& index = data.Index;

		cbvDesc.BufferLocation = m_objectConstant.GetGPUVirtualAddress(index);
		auto handleMappedData = m_objectConstant.GetHandleMappedData(index);
		XMStoreFloat4x4(&handleMappedData->World, XMMatrixIdentity());
		XMStoreFloat4x4(&handleMappedData->TexTransform, XMMatrixIdentity());

		m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

		heapHandle.Offset(1, heap_size);
	}

	// Material texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	for (const auto& drawData : m_drawDatas)
	{
		const auto& name = drawData.first;
		const auto& data = drawData.second;
		const auto& index = data.Index;

		auto texture = m_loaders[name].Texture;
		texture = texture != nullptr ? texture : m_whiteTex;
		auto texDesc = texture->GetDesc();
		srvDesc.Format = texDesc.Format;

		m_device->CreateShaderResourceView(texture.Get(), &srvDesc, heapHandle);
		
		heapHandle.Offset(1, heap_size);
	}

	return true;
}

bool PrimitiveManager::Impl::Has(const std::string& name)
{
	return m_loaders.count(name);
}

//
/*---------INTERFACE METHOD-----------*/
//

PrimitiveManager::PrimitiveManager():m_impl(new Impl())
{
}

PrimitiveManager::PrimitiveManager(ID3D12Device* pDevice):m_impl(new Impl(pDevice))
{
}

PrimitiveManager::~PrimitiveManager()
{
	SAFE_DELETE(m_impl);
}

bool PrimitiveManager::SetDevice(ID3D12Device* pDevice)
{
	assert(pDevice);
	if (!pDevice) return false;
	IMPL.m_device = pDevice;
	return true;
}

bool PrimitiveManager::SetWorldPassConstantGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS worldPassConstantGpuAddress)
{
	assert(IMPL.m_device);
	if (IMPL.m_device == nullptr) return false;
	if (worldPassConstantGpuAddress <= 0) return false;

	IMPL.m_worldPassAdress = worldPassConstantGpuAddress;
}

bool PrimitiveManager::SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer)
{
	assert(IMPL.m_device);
	if (pShadowDepthBuffer == nullptr) return false;

	D12Helper::CreateDescriptorHeap(IMPL.m_device.Get(), IMPL.m_depthHeap, 1,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	auto rsDes = pShadowDepthBuffer->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	/*--------------EASY TO BECOME BUG----------------*/
	// the type of shadow depth buffer is R32_TYPELESS
	// In able to shader resource view to read shadow depth buffer
	// => Set format SRV to DXGI_FORMAT_R32_FLOAT
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	/*-------------------------------------------------*/

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(IMPL.m_depthHeap->GetCPUDescriptorHandleForHeapStart());
	IMPL.m_device->CreateShaderResourceView(pShadowDepthBuffer, &srvDesc, heapHandle);

	return true;
}

bool PrimitiveManager::SetViewDepth(ID3D12Resource* pViewDepthBuffer)
{
	assert(IMPL.m_device);
	if (pViewDepthBuffer == nullptr) return false;

	auto rsDes = pViewDepthBuffer->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

	/*--------------EASY TO BECOME BUG----------------*/
	// the type of shadow depth buffer is R32_TYPELESS
	// In able to shader resource view to read shadow depth buffer
	// => Set format SRV to DXGI_FORMAT_R32_FLOAT
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	/*-------------------------------------------------*/

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(IMPL.m_depthHeap->GetCPUDescriptorHandleForHeapStart());
	heapHandle.Offset(1, IMPL.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	IMPL.m_device->CreateShaderResourceView(pViewDepthBuffer, &srvDesc, heapHandle);

	return true;
}

bool PrimitiveManager::Create(const std::string& name, Geometry::Mesh primitive, 
	D3D12_GPU_VIRTUAL_ADDRESS materialCBGpuAddress, ID3D12Resource* pTexture)
{
	assert(!IMPL.Has(name));
	auto& loader = IMPL.m_loaders[name];
	loader.Primitive = primitive;
	loader.MaterialCBAdress = materialCBGpuAddress;
	loader.Texture = pTexture;

	return true;
}

bool PrimitiveManager::Init(ID3D12GraphicsCommandList* cmdList)
{
	if (IMPL.m_isInitDone) return true;
	if (!IMPL.m_device) return false;
	if (!IMPL.m_worldPassAdress) return false;
	if (!IMPL.m_depthHeap) return false;

	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	for (auto& loader : IMPL.m_loaders)
	{
		auto& name = loader.first;
		auto& data = loader.second;
		auto& primitive = data.Primitive;

		IMPL.m_mesh.DrawArgs[name].StartIndexLocation = indexCount;
		IMPL.m_mesh.DrawArgs[name].BaseVertexLocation = vertexCount;
		IMPL.m_mesh.DrawArgs[name].IndexCount = primitive.indices.size();
		indexCount += primitive.indices.size();
		vertexCount += primitive.vertices.size();
	}

	IMPL.m_mesh.Indices16.reserve(indexCount);
	IMPL.m_mesh.Vertices.reserve(vertexCount);

	// Add all vertices and indices
	for (auto& loader : IMPL.m_loaders)
	{
		auto& name = loader.first;
		auto& data = loader.second;
		auto& primitive = data.Primitive;

		for (const auto& index : primitive.indices)
			IMPL.m_mesh.Indices16.push_back(index);

		for (const auto& vertex : primitive.vertices)
			IMPL.m_mesh.Vertices.push_back(vertex);
	}

	uint16_t index = -1;
	for (auto& loader : IMPL.m_loaders)
	{
		auto& name = loader.first;
		auto& data = loader.second;
		
		auto& drawData = IMPL.m_drawDatas[name];
		drawData.Index = ++index;
		drawData.MaterialCBAddress = data.MaterialCBAdress;
	}

	IMPL.CreateObjectHeap();

	IMPL.m_mesh.CreateBuffers(IMPL.m_device.Get(), cmdList);
	IMPL.m_mesh.CreateViews();
	IMPL.m_loaders.clear();

	return true;
}

bool PrimitiveManager::ClearSubresources()
{
	IMPL.m_mesh.ClearSubresource();
	return true;
}

bool PrimitiveManager::Move(const std::string& name, float x, float y, float z)
{
	const auto& drawData = IMPL.m_drawDatas[name];
	const auto& index = drawData.Index;
	auto handleMappedData = IMPL.m_objectConstant.GetHandleMappedData(index);
	XMStoreFloat4x4(&handleMappedData->World, XMMatrixTranslation(x, y, z));
	return true;
}

bool PrimitiveManager::ScaleTexture(const std::string& name, float u, float v)
{
	const auto& drawData = IMPL.m_drawDatas[name];
	const auto& index = drawData.Index;
	auto handleMappedData = IMPL.m_objectConstant.GetHandleMappedData(index);
	XMStoreFloat4x4(&handleMappedData->TexTransform, XMMatrixScaling(u, v, 0.0f));
	return true;
}

void PrimitiveManager::Update(const float& deltaTime)
{
}

void PrimitiveManager::Render(ID3D12GraphicsCommandList* pCmdList)
{
	// Set Input Assembler
	pCmdList->IASetVertexBuffers(0, 1, &IMPL.m_mesh.VertexBufferView);
	pCmdList->IASetIndexBuffer(&IMPL.m_mesh.IndexBufferView);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// World constant
	pCmdList->SetGraphicsRootConstantBufferView(0, IMPL.m_worldPassAdress);
	
	// Depth buffers
	pCmdList->SetDescriptorHeaps(1, IMPL.m_depthHeap.GetAddressOf());
	pCmdList->SetGraphicsRootDescriptorTable(1, IMPL.m_depthHeap->GetGPUDescriptorHandleForHeapStart());

	// Object Constant
	pCmdList->SetDescriptorHeaps(1, IMPL.m_objectHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE objectHeapHandle(IMPL.m_objectHeap->GetGPUDescriptorHandleForHeapStart());
	const auto heap_size = IMPL.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	auto texHeapHandle = objectHeapHandle;
	texHeapHandle.Offset(IMPL.m_drawDatas.size(), heap_size);

	for (const auto& drawData : IMPL.m_drawDatas)
	{
		const auto& name = drawData.first;
		const auto& data = drawData.second;
		const auto& index = data.Index;
		const auto& materialCBGpuAddress = data.MaterialCBAddress;
		const auto& drawArgs = IMPL.m_mesh.DrawArgs[name];

		// Set table for object constant
		pCmdList->SetGraphicsRootDescriptorTable(2, objectHeapHandle);
		objectHeapHandle.Offset(1, heap_size);
		// Set table for texture shader
		pCmdList->SetGraphicsRootDescriptorTable(3, texHeapHandle);
		texHeapHandle.Offset(1, heap_size);
		// Set table for material constant
		pCmdList->SetGraphicsRootConstantBufferView(4, materialCBGpuAddress);
		pCmdList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.StartIndexLocation, drawArgs.BaseVertexLocation, 0);
	}
}

void PrimitiveManager::RenderDepth(ID3D12GraphicsCommandList* pCmdList)
{
	// Set Input Assembler
	pCmdList->IASetVertexBuffers(0, 1, &IMPL.m_mesh.VertexBufferView);
	pCmdList->IASetIndexBuffer(&IMPL.m_mesh.IndexBufferView);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// World constant
	pCmdList->SetGraphicsRootConstantBufferView(0, IMPL.m_worldPassAdress);

	// Object Constant
	pCmdList->SetDescriptorHeaps(1, IMPL.m_objectHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE objectHeapHandle(IMPL.m_objectHeap->GetGPUDescriptorHandleForHeapStart());
	const auto heap_size = IMPL.m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	for (auto& primitive : IMPL.m_mesh.DrawArgs)
	{
		auto& drawArgs = primitive.second;
		pCmdList->SetGraphicsRootDescriptorTable(1, objectHeapHandle);
		objectHeapHandle.Offset(1, heap_size);
		pCmdList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.StartIndexLocation, drawArgs.BaseVertexLocation, 0);
	}
}

PrimitiveManager::PrimitiveManager(const PrimitiveManager&)
{
}

void PrimitiveManager::operator=(const PrimitiveManager&)
{
}

