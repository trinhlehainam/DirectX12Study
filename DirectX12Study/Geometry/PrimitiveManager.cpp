#include "PrimitiveManager.h"
#include "../Utility/D12Helper.h"
#include <cassert>

using Microsoft::WRL::ComPtr;

PrimitiveManager::PrimitiveManager()
{
}

PrimitiveManager::PrimitiveManager(ID3D12Device* pDevice):m_device(pDevice)
{
}

PrimitiveManager::~PrimitiveManager()
{
	m_device = nullptr;
}

bool PrimitiveManager::SetDevice(ID3D12Device* pDevice)
{
	assert(pDevice);
	if (!pDevice) return false;
	m_device = pDevice;
	return true;
}

bool PrimitiveManager::SetWorldPassConstant(ID3D12Resource* pWorldPassConstant, size_t bufferSize)
{
	assert(m_device);
	if (m_device == nullptr) return false;
	if (pWorldPassConstant == nullptr) return false;
	if (bufferSize == 0) return false;

	m_worldPassAdress = pWorldPassConstant->GetGPUVirtualAddress();
}

bool PrimitiveManager::SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer)
{
	assert(m_device);
	if (pShadowDepthBuffer == nullptr) return false;

	D12Helper::CreateDescriptorHeap(m_device, m_depthHeap, 1,
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_depthHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateShaderResourceView(pShadowDepthBuffer, &srvDesc, heapHandle);

	return true;
}

bool PrimitiveManager::SetViewDepth(ID3D12Resource* pViewDepthBuffer)
{
	assert(m_device);
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_depthHeap->GetCPUDescriptorHandleForHeapStart());
	heapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
	m_device->CreateShaderResourceView(pViewDepthBuffer, &srvDesc, heapHandle);

	return true;
}

bool PrimitiveManager::Create(const std::string name, Geometry::Mesh primitive)
{
	assert(!m_primitives.count(name));
	m_primitives[name] = primitive;
	return true;
}

bool PrimitiveManager::Init(ID3D12GraphicsCommandList* cmdList)
{
	if (m_isInitDone) return true;
	if (!m_device) return false;
	if (!m_worldPassAdress) return false;
	if (!m_depthHeap) return false;
	if (!CreatePipeline()) return false;

	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	for (auto& primitive : m_primitives)
	{
		auto& data = primitive.second;
		auto& name = primitive.first;

		m_mesh.DrawArgs[name].StartIndexLocation = indexCount;
		m_mesh.DrawArgs[name].BaseVertexLocation = vertexCount;
		m_mesh.DrawArgs[name].IndexCount = data.indices.size();
		indexCount += data.indices.size();
		vertexCount += data.vertices.size();
	}

	m_mesh.Indices16.reserve(indexCount);
	m_mesh.Vertices.reserve(vertexCount);

	// Add all vertices and indices
	for (auto& primitive : m_primitives)
	{
		auto& name = primitive.first;;
		auto& data = primitive.second;

		for (const auto& index : data.indices)
			m_mesh.Indices16.push_back(index);

		for (const auto& vertex : data.vertices)
			m_mesh.Vertices.push_back(vertex);
	}

	m_mesh.CreateBuffers(m_device, cmdList);
	m_mesh.CreateViews();

	return true;
}

bool PrimitiveManager::ClearSubresources()
{
	m_mesh.ClearSubresource();
	return true;
}

void PrimitiveManager::Update(const float& deltaTime)
{
}

void PrimitiveManager::Render(ID3D12GraphicsCommandList* pCmdList)
{
	pCmdList->SetPipelineState(m_pipeline.Get());
	pCmdList->SetGraphicsRootSignature(m_rootSig.Get());

	// Set Input Assembler
	pCmdList->IASetVertexBuffers(0, 1, &m_mesh.VertexBufferView);
	pCmdList->IASetIndexBuffer(&m_mesh.IndexBufferView);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// World constant
	pCmdList->SetGraphicsRootConstantBufferView(0, m_worldPassAdress);
	
	// Depth buffers
	pCmdList->SetDescriptorHeaps(1, m_depthHeap.GetAddressOf());
	pCmdList->SetGraphicsRootDescriptorTable(1, m_depthHeap->GetGPUDescriptorHandleForHeapStart());

	for (auto& primitive : m_mesh.DrawArgs)
	{
		auto& drawArgs = primitive.second;
		pCmdList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.StartIndexLocation, drawArgs.BaseVertexLocation, 0);
	}
		
}

void PrimitiveManager::RenderDepth(ID3D12GraphicsCommandList* pCmdList)
{
	// World constant
	pCmdList->SetGraphicsRootConstantBufferView(0, m_worldPassAdress);

	// Set Input Assembler
	pCmdList->IASetVertexBuffers(0, 1, &m_mesh.VertexBufferView);
	pCmdList->IASetIndexBuffer(&m_mesh.IndexBufferView);
	pCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	for (auto& primitive : m_mesh.DrawArgs)
	{
		auto& drawArgs = primitive.second;
		pCmdList->DrawIndexedInstanced(drawArgs.IndexCount, 1, drawArgs.StartIndexLocation, drawArgs.BaseVertexLocation, 0);
	}
}

bool PrimitiveManager::CreatePipeline()
{
	if (!CreateRootSignature()) return false;
	if (!CreatePipelineStateObject()) return false;
	return true;
}

bool PrimitiveManager::CreateRootSignature()
{
	HRESULT result = S_OK;
	//Root Signature
	D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
	rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	// Descriptor table
	D3D12_DESCRIPTOR_RANGE range[1] = {};
	D3D12_ROOT_PARAMETER parameter[2] = {};

	// world pass constant
	CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(parameter[0], 0, 0, D3D12_SHADER_VISIBILITY_ALL);

	// shadow depth buffer
	range[0] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // range type
		1,                                      // number of descriptors
		0);                                     // base shader register
	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[1], 1, &range[0], D3D12_SHADER_VISIBILITY_ALL);

	// object constant


	rtSigDesc.pParameters = parameter;
	rtSigDesc.NumParameters = _countof(parameter);

	D3D12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
	CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0], 0,
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

bool PrimitiveManager::CreatePipelineStateObject()
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
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
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
