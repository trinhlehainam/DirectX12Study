#include "PMDManager.h"
#include "PMDModel.h"
#include "../DirectX12/UploadBuffer.h"

#include <cassert>

PMDManager::PMDManager()
{
	m_updateFunc = &PMDManager::SleepUpdate;
	m_renderFunc = &PMDManager::SleepRender;
}

PMDManager::PMDManager(ID3D12Device* pDevice):m_device(pDevice)
{
	m_updateFunc = &PMDManager::SleepUpdate;
	m_renderFunc = &PMDManager::SleepRender;
}

PMDManager::~PMDManager()
{
	m_device = nullptr;
	m_whiteTexture = nullptr;
	m_blackTexture = nullptr;
	m_gradTexture = nullptr;
}

bool PMDManager::SetDefaultBuffer(ID3D12Resource* pWhiteTexture, ID3D12Resource* pBlackTexture, ID3D12Resource* pGradTexture)
{
	if (!pWhiteTexture || !pBlackTexture || !pGradTexture) return false;

	m_whiteTexture = pWhiteTexture;
	m_blackTexture = pBlackTexture;
	m_gradTexture = pGradTexture;

	return true;
}

bool PMDManager::Init()
{
	if (m_isInitDone) return true;
	if (!m_device) return false;
	if (!m_worldPCBHeap) return false;
	if (!m_shadowDepthHeap) return false;

	if (!CheckDefaultBuffers()) return false;
	if (!CreatePipeline()) return false;

	// Init all models
	for (auto& model : m_models)
	{
		model.second.GetDefaultTexture(m_whiteTexture, m_blackTexture, m_gradTexture);
		model.second.CreateModel();
	}

	size_t indexCount = 0;
	size_t vertexCount = 0;
	// Loop for calculate size of indices of all models
	// And save baseIndex of each model for Render Usage
	for (auto& model : m_models)
	{
		auto& data = model.second;
		m_meshes.meshIndex[model.first].baseIndex = indexCount;
		indexCount += data.indices_.size();
		vertexCount += data.vertices_.size();
	}
	m_meshes.indices.reserve(indexCount);
	m_meshes.vertices.reserve(vertexCount);

	// Add all vertices and indices to PMDMeshes
	for (auto& data : m_models)
	{
		auto& model = data.second;

		for (const auto& indice : model.indices_)
			m_meshes.indices.push_back(indice);

		for (const auto& vertex : model.vertices_)
			m_meshes.vertices.push_back(vertex);
	}

	m_meshes.CreateVertexBufferView(m_device);
	m_meshes.CreateIndexBufferView(m_device);
	
	m_isInitDone = true;
	m_updateFunc = &PMDManager::PrivateUpdate;
	m_renderFunc = &PMDManager::PrivateRender;

	return true;
}

bool PMDManager::SetDevice(ID3D12Device* pDevice)
{
    if (pDevice == nullptr) return false;
    m_device = pDevice;
    return false;
}

bool PMDManager::SetWorldPassConstant(ID3D12Resource* pWorldPassConstant , size_t bufferSize)
{
	assert(m_device);
	if (m_device == nullptr) return false;
	if (pWorldPassConstant == nullptr) return false;
	if (bufferSize == 0) return false;

	Dx12Helper::CreateDescriptorHeap(m_device, m_worldPCBHeap, 
		1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = pWorldPassConstant->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_worldPCBHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

	return true;
}

bool PMDManager::SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer)
{
	assert(m_device);
	if (pShadowDepthBuffer == nullptr) return false;

	Dx12Helper::CreateDescriptorHeap(m_device, m_shadowDepthHeap, 1,
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

bool PMDManager::IsInitialized()
{
	return m_isInitDone;
}

bool PMDManager::Add(const std::string& name)
{
	assert(m_device);
	if (m_models.count(name)) return false;
	m_models[name].m_device = m_device;
	return true;
}

PMDModel& PMDManager::Get(const std::string& name)
{
	assert(m_models.count(name));
	return m_models[name];
}

void PMDManager::Update(const float& deltaTime)
{
	(this->*m_updateFunc)(deltaTime);
}

void PMDManager::Render(ID3D12GraphicsCommandList* pGraphicsCmdList)
{
	(this->*m_renderFunc)(pGraphicsCmdList);
}

void PMDManager::SleepUpdate(const float& deltaTime)
{
	assert(m_isInitDone);
}

void PMDManager::SleepRender(ID3D12GraphicsCommandList* pGraphicsCmdList)
{
	assert(m_isInitDone);
}

void PMDManager::PrivateUpdate(const float& deltaTime)
{

}

void PMDManager::PrivateRender(ID3D12GraphicsCommandList* pGraphicsCmdList)
{
	pGraphicsCmdList->SetPipelineState(m_pipeline.Get());
	pGraphicsCmdList->SetGraphicsRootSignature(m_rootSig.Get());

	// Set Input Assembler
	pGraphicsCmdList->IASetVertexBuffers(0, 1, &m_meshes.vbview);
	pGraphicsCmdList->IASetIndexBuffer(&m_meshes.ibview);
	pGraphicsCmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set world pass constant
	pGraphicsCmdList->SetDescriptorHeaps(1, m_worldPCBHeap.GetAddressOf());
	pGraphicsCmdList->SetGraphicsRootDescriptorTable(0, m_worldPCBHeap->GetGPUDescriptorHandleForHeapStart());

	// Set shadow depth
	pGraphicsCmdList->SetDescriptorHeaps(1, m_shadowDepthHeap.GetAddressOf());
	pGraphicsCmdList->SetGraphicsRootDescriptorTable(1, m_shadowDepthHeap->GetGPUDescriptorHandleForHeapStart());

	// Render all models
	for (auto& model : m_models)
	{
		auto& name = model.first;
		auto& data = model.second;
		auto& index = m_meshes.meshIndex[name].baseIndex;

		data.Render(pGraphicsCmdList, index);
	}
}

bool PMDManager::CreatePipeline()
{
	if (!CreateRootSignature()) return false;
	if (!CreatePipelineStateObject()) return false;
	return true;
}

bool PMDManager::CreateRootSignature()
{
	HRESULT result = S_OK;
	//Root Signature
	D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
	rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	CD3DX12_ROOT_PARAMETER parameter[4] = {};
	CD3DX12_DESCRIPTOR_RANGE range[5] = {};

	// World Pass Constant
	range[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	parameter[0].InitAsDescriptorTable(1, &range[0]);

	// Shadow Depth
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
	parameter[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);
	
	// Object Constant
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	parameter[2].InitAsDescriptorTable(1, &range[2], D3D12_SHADER_VISIBILITY_VERTEX);
	
	// Material
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 1);
	parameter[3].InitAsDescriptorTable(2, &range[3], D3D12_SHADER_VISIBILITY_PIXEL);

	rtSigDesc.pParameters = parameter;
	rtSigDesc.NumParameters = _countof(parameter);

	//サンプラらの定義、サンプラとはUVが0未満とか1超えとかのときの
	//動きyや、UVをもとに色をとってくるときのルールを指定するもの
	D3D12_STATIC_SAMPLER_DESC samplerDesc[3] = {};
	//WRAPは繰り返しを表す。
	CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0],
		0,                                  // shader register location
		D3D12_FILTER_MIN_MAG_MIP_POINT);    // Filter     

	samplerDesc[1] = samplerDesc[0];
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1;
	
	samplerDesc[2] = samplerDesc[0];
	samplerDesc[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	samplerDesc[2].ShaderRegister = 2;
	samplerDesc[2].MaxAnisotropy = 1;
	samplerDesc[2].MipLODBias = 0.0f;

	rtSigDesc.pStaticSamplers = samplerDesc;
	rtSigDesc.NumStaticSamplers = _countof(samplerDesc);

	ComPtr<ID3DBlob> rootSigBlob;
	ComPtr<ID3DBlob> errBlob;
	Dx12Helper::ThrowIfFailed(D3D12SerializeRootSignature(&rtSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
		&rootSigBlob,
		&errBlob));
	Dx12Helper::OutputFromErrorBlob(errBlob);
	Dx12Helper::ThrowIfFailed(m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(), IID_PPV_ARGS(m_rootSig.ReleaseAndGetAddressOf())));

	return true;
}

bool PMDManager::CreatePipelineStateObject()
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
	"NORMAL",
	0,
	DXGI_FORMAT_R32G32B32_FLOAT,
	0,
	D3D12_APPEND_ALIGNED_ELEMENT,
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
	0
	},
		// UV layout
		{
		"TEXCOORD",
		0,
		DXGI_FORMAT_R32G32_FLOAT,                     // float2 -> [2D array] R32G32
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
		},
		{
		"BONENO",
		0,
		DXGI_FORMAT_R16G16_UINT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
		},
		{
		"WEIGHT",
		0,
		DXGI_FORMAT_R32_FLOAT,
		0,
		D3D12_APPEND_ALIGNED_ELEMENT,
		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
		0
		}
	};


	// Input Assembler
	psoDesc.InputLayout.NumElements = _countof(layout);
	psoDesc.InputLayout.pInputElementDescs = layout;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	// Vertex Shader
	ComPtr<ID3DBlob> vsBlob = Dx12Helper::CompileShaderFromFile(L"shader/vs.hlsl", "VS", "vs_5_1");
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

	// Rasterizer
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Pixel Shader
	ComPtr<ID3DBlob> psBlob = Dx12Helper::CompileShaderFromFile(L"shader/ps.hlsl", "PS", "ps_5_1");
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

	// Other set up

	// Depth/Stencil
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

	psoDesc.NodeMask = 0;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleDesc.Quality = 0;
	psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

	// Output set up
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Blend
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;     //※ color : ABGR

	// Root Signature
	psoDesc.pRootSignature = m_rootSig.Get();

	Dx12Helper::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, 
		IID_PPV_ARGS(m_pipeline.GetAddressOf())));

	return true;
}

bool PMDManager::CheckDefaultBuffers()
{
	return m_whiteTexture && m_blackTexture && m_gradTexture;
}

