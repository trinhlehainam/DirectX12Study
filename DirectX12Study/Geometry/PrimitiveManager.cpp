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

	D12Helper::CreateDescriptorHeap(m_device, m_worldPCBHeap,
		1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = pWorldPassConstant->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_worldPCBHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateConstantBufferView(&cbvDesc, heapHandle);
}

bool PrimitiveManager::SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer)
{
	assert(m_device);
	if (pShadowDepthBuffer == nullptr) return false;

	D12Helper::CreateDescriptorHeap(m_device, m_shadowDepthHeap, 1,
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

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_shadowDepthHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateShaderResourceView(pShadowDepthBuffer, &srvDesc, heapHandle);

	return true;
}

bool PrimitiveManager::Init(ID3D12GraphicsCommandList* cmdList)
{
	if (m_isInitDone) return true;
	if (!m_device) return false;
	if (!m_worldPCBHeap) return false;
	if (!m_shadowDepthHeap) return false;

	return true;
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
	D3D12_DESCRIPTOR_RANGE range[2] = {};

	// world pass constant
	range[0] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
		1,                                      // number of descriptors
		0);                                     // base shader register

	// shadow depth buffer
	range[1] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // range type
		1,                                      // number of descriptors
		0);                                     // base shader register

	// object constant

	D3D12_ROOT_PARAMETER parameter[1] = {};
	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[0], 2, &range[0], D3D12_SHADER_VISIBILITY_ALL);

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
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
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
