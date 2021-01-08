#include "PrimitiveManager.h"

#include <cassert>
#include <unordered_map>

#include "../Utility/D12Helper.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/UploadBuffer.h"
#include "Mesh.h"

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

	bool CreatePipeline();
	bool CreateRootSignature();
	bool CreatePipelineStateObject();
	bool CreateObjectHeap();
	bool Has(const std::string& name);
private:
	bool m_isInitDone = false;
	// Device from engine
	ID3D12Device* m_device = nullptr;

	ComPtr<ID3D12RootSignature> m_rootSig = nullptr;
	ComPtr<ID3D12PipelineState> m_pipeline = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS m_worldPassAdress = 0;

	// Descriptor heap stores descriptor of depth buffer
	// Use for binding resource of engine to this pipeline
	ComPtr<ID3D12DescriptorHeap> m_depthHeap = nullptr;
	uint16_t m_depthBufferCount = 0;

	UploadBuffer<XMFLOAT4X4> m_objectConstant;
	ComPtr<ID3D12DescriptorHeap> m_objectHeap = nullptr;

	ComPtr<ID3D12DescriptorHeap> m_textureHeap = nullptr;
private:
	Mesh<Geometry::Vertex> m_mesh;
	struct Loader
	{
		Geometry::Mesh Primitive;
		D3D12_GPU_VIRTUAL_ADDRESS MaterialCBAdress;
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

bool PrimitiveManager::Impl::CreatePipeline()
{
	if (!CreateRootSignature()) return false;
	if (!CreatePipelineStateObject()) return false;
	return true;
}

bool PrimitiveManager::Impl::CreateRootSignature()
{
	HRESULT result = S_OK;
	//Root Signature
	D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
	rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Descriptor table
	D3D12_DESCRIPTOR_RANGE range[3] = {};
	D3D12_ROOT_PARAMETER parameter[5] = {};

	// world pass constant
	CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(parameter[0], 0, 0, D3D12_SHADER_VISIBILITY_ALL);

	// shadow depth buffer
	range[0] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // range type
		1,                                      // number of descriptors
		0);                                     // base shader register
	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[1], 1, &range[0], D3D12_SHADER_VISIBILITY_ALL);

	// object constant
	range[1] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
		1,                                      // number of descriptors
		1);                                     // base shader register
	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[2], 1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

	// material constant
	CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(parameter[3], 2, 0, D3D12_SHADER_VISIBILITY_ALL);

	// texture
	range[2] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
		1,
		1);
	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[4], 1, &range[2],D3D12_SHADER_VISIBILITY_PIXEL);

	rtSigDesc.pParameters = parameter;
	rtSigDesc.NumParameters = _countof(parameter);

	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};

	CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0], 0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[1], 1,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,
		D3D12_TEXTURE_ADDRESS_MODE_BORDER);

	rtSigDesc.pStaticSamplers = samplerDesc;
	rtSigDesc.NumStaticSamplers = _countof(samplerDesc);

	ComPtr<ID3DBlob> rootSigBlob;
	ComPtr<ID3DBlob> errBlob;
	result = D3D12SerializeRootSignature(&rtSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
		&rootSigBlob,
		&errBlob);
	D12Helper::OutputFromErrorBlob(errBlob);
	assert(SUCCEEDED(result));
	result = m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
		IID_PPV_ARGS(m_rootSig.GetAddressOf()));
	assert(SUCCEEDED(result));

	return true;
}
bool PrimitiveManager::Impl::CreatePipelineStateObject()
{
	HRESULT result = S_OK;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	//IA(Input-Assembler...つまり頂点入力)
	//まず入力レイアウト（ちょうてんのフォーマット）

	D3D12_INPUT_ELEMENT_DESC layout[] = {
	{
	"POSITION",                                   //semantic
	0,                                            //semantic index(配列の場合に配列番号を入れる)
	DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
	0,                                            //スロット番号（頂点データが入ってくる入口地番号）
	0,                                            //このデータが何バイト目から始まるのか
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
	0},
	{
	"NORMAL",                                   //semantic
	0,                                            //semantic index(配列の場合に配列番号を入れる)
	DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
	0,                                            //スロット番号（頂点データが入ってくる入口地番号）
	D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
	0
	},
{
	"TANGENT",                                   //semantic
	0,                                            //semantic index(配列の場合に配列番号を入れる)
	DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
	0,                                            //スロット番号（頂点データが入ってくる入口地番号）
	D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
	0
	},
	{
	"TEXCOORD",                                   //semantic
	0,                                            //semantic index(配列の場合に配列番号を入れる)
	DXGI_FORMAT_R32G32_FLOAT,                  // float3 -> [3D array] R32G32B32
	0,                                            //スロット番号（頂点データが入ってくる入口地番号）
	D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
	0
	},
	};

	// Input Assembler
	psoDesc.InputLayout.NumElements = _countof(layout);
	psoDesc.InputLayout.pInputElementDescs = layout;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// Vertex Shader
	ComPtr<ID3DBlob> vsBlob = D12Helper::CompileShaderFromFile(L"Shader/primitiveVS.hlsl", "primitiveVS", "vs_5_1");
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	// Rasterizer
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	//psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	// Pixel Shader
	ComPtr<ID3DBlob> psBlob = D12Helper::CompileShaderFromFile(L"Shader/primitivePS.hlsl", "primitivePS", "ps_5_1");
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
	// Other set up
	psoDesc.NodeMask = 0;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;
	// Output set up
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Blend
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	// Root Signature
	psoDesc.pRootSignature = m_rootSig.Get();
	result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pipeline.GetAddressOf()));
	assert(SUCCEEDED(result));

	return true;
}

bool PrimitiveManager::Impl::CreateObjectHeap()
{
	const auto num_primitive = m_drawDatas.size();
	D12Helper::CreateDescriptorHeap(m_device, m_objectHeap, num_primitive, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_objectConstant.Create(m_device, num_primitive, true);

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_objectHeap->GetCPUDescriptorHandleForHeapStart());
	const auto heap_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = m_objectConstant.ElementSize();
	for (const auto& drawData : m_drawDatas)
	{
		const auto& name = drawData.first;
		const auto& data = drawData.second;
		const auto& index = data.Index;

		cbvDesc.BufferLocation = m_objectConstant.GetGPUVirtualAddress(index);
		auto handleMappedData = m_objectConstant.GetHandleMappedData(index);
		XMStoreFloat4x4(handleMappedData, XMMatrixIdentity());

		m_device->CreateConstantBufferView(&cbvDesc, heapHandle);
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

	D12Helper::CreateDescriptorHeap(IMPL.m_device, IMPL.m_depthHeap, 1,
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

	

	if(IMPL.m_textureHeap == nullptr)
		D12Helper::CreateDescriptorHeap(IMPL.m_device, IMPL.m_textureHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(IMPL.m_textureHeap->GetCPUDescriptorHandleForHeapStart());
	if (pTexture != nullptr)
	{
		auto texDesc = pTexture->GetDesc();
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = texDesc.Format;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		IMPL.m_device->CreateShaderResourceView(pTexture, &srvDesc, heapHandle);
	}

	return true;
}

bool PrimitiveManager::Init(ID3D12GraphicsCommandList* cmdList)
{
	if (IMPL.m_isInitDone) return true;
	if (!IMPL.m_device) return false;
	if (!IMPL.m_worldPassAdress) return false;
	if (!IMPL.m_depthHeap) return false;
	if (!IMPL.CreatePipeline()) return false;

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

	IMPL.m_mesh.CreateBuffers(IMPL.m_device, cmdList);
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
	XMStoreFloat4x4(handleMappedData, XMMatrixTranslation(x, y, z));
	return true;
}

void PrimitiveManager::Update(const float& deltaTime)
{
}

void PrimitiveManager::Render(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->SetPipelineState(IMPL.m_pipeline.Get());
	pCmdList->SetGraphicsRootSignature(IMPL.m_rootSig.Get());

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

	for (const auto& drawData : IMPL.m_drawDatas)
	{
		const auto& name = drawData.first;
		const auto& data = drawData.second;
		const auto& index = data.Index;
		const auto& materialCBGpuAddress = data.MaterialCBAddress;
		const auto& drawArgs = IMPL.m_mesh.DrawArgs[name];

		pCmdList->SetGraphicsRootDescriptorTable(2, objectHeapHandle);
		pCmdList->SetGraphicsRootConstantBufferView(3, materialCBGpuAddress);
		// Set up texture table
		CD3DX12_GPU_DESCRIPTOR_HANDLE texHeapHandle(IMPL.m_textureHeap->GetGPUDescriptorHandleForHeapStart());
		pCmdList->SetDescriptorHeaps(1, IMPL.m_textureHeap.GetAddressOf());
		pCmdList->SetGraphicsRootDescriptorTable(4, texHeapHandle);
		pCmdList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.StartIndexLocation, drawArgs.BaseVertexLocation, 0);
		objectHeapHandle.Offset(1, heap_size);
	}
}

void PrimitiveManager::RenderDepth(ID3D12GraphicsCommandList* pCmdList)
{
	// World constant
	pCmdList->SetGraphicsRootConstantBufferView(0, IMPL.m_worldPassAdress);

	// Set Input Assembler
	pCmdList->IASetVertexBuffers(0, 1, &IMPL.m_mesh.VertexBufferView);
	pCmdList->IASetIndexBuffer(&IMPL.m_mesh.IndexBufferView);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& primitive : IMPL.m_mesh.DrawArgs)
	{
		auto& drawArgs = primitive.second;
		pCmdList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.StartIndexLocation, drawArgs.BaseVertexLocation, 0);
	}
}

PrimitiveManager::PrimitiveManager(const PrimitiveManager&)
{
}

void PrimitiveManager::operator=(const PrimitiveManager&)
{
}

