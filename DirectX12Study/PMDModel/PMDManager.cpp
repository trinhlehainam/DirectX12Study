#include "PMDManager.h"
#include "PMDModel.h"
#include "../DirectX12/UploadBuffer.h"

#include <cassert>

PMDManager::PMDManager()
{
	m_updateFunc = &PMDManager::Sleep;
	m_renderFunc = &PMDManager::Sleep;
}

PMDManager::PMDManager(ID3D12Device* pDevice):m_device(pDevice)
{
	m_updateFunc = &PMDManager::Sleep;
	m_renderFunc = &PMDManager::Sleep;
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
	if (!m_device) return false;
	if (!CheckDefaultBuffers()) return false;
	if (!CreatePipeline()) return false;

    
	if (!m_worldPCBHeap) return false;
	if (!m_shadowDepthHeap) return false;

	

	size_t indicesCount = 0;
	// Loop for calculate size of indices of all models
	// And save baseIndex of each model for Render Usage
	for (auto& data : m_models)
	{
		auto& model = data.second;
		m_meshes.meshIndex[data.first].baseIndex = indicesCount;
		indicesCount += model.indices_.size();
	}
	m_meshes.indices.reserve(indicesCount);

	// Add all indices to Meshes
	for (auto& data : m_models)
	{
		auto& model = data.second;
		for (const auto& indice : model.indices_)
		{
			m_meshes.indices.push_back(indice);
		}
	}
	
	m_isInitDone = true;
	m_updateFunc = &PMDManager::PrivateUpdate;
	m_updateFunc = &PMDManager::PrivateUpdate;

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
	srvDesc.Format = rsDes.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_shadowDepthHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateShaderResourceView(pShadowDepthBuffer, &srvDesc, heapHandle);

	return false;
}

bool PMDManager::Add(const std::string& name)
{
	if (m_models.count(name)) return false;
	m_models[name].m_device = m_device;
	return true;
}

PMDModel& PMDManager::Get(const std::string& name)
{
	assert(m_models.count(name));
	return m_models[name];
}

void PMDManager::Update()
{
	(this->*m_updateFunc)();
}

void PMDManager::Sleep()
{
	assert(m_isInitDone);
}

void PMDManager::PrivateRender()
{
}

void PMDManager::PrivateUpdate()
{
}

void PMDManager::Render()
{
	(this->*m_renderFunc)();
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

	//�T���v����̒�`�A�T���v���Ƃ�UV��0�����Ƃ�1�����Ƃ��̂Ƃ���
	//����y��AUV�����ƂɐF���Ƃ��Ă���Ƃ��̃��[�����w�肷�����
	D3D12_STATIC_SAMPLER_DESC samplerDesc[3] = {};
	//WRAP�͌J��Ԃ���\���B
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
		D3D_ROOT_SIGNATURE_VERSION_1_0,             //�� 
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
	//IA(Input-Assembler...�܂蒸�_����)
	//�܂����̓��C�A�E�g�i���傤�Ă�̃t�H�[�}�b�g�j

	D3D12_INPUT_ELEMENT_DESC layout[] = {
	{
	"POSITION",                                   //semantic
	0,                                            //semantic index(�z��̏ꍇ�ɔz��ԍ�������)
	DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
	0,                                            //�X���b�g�ԍ��i���_�f�[�^�������Ă�������n�ԍ��j
	0,                                            //���̃f�[�^�����o�C�g�ڂ���n�܂�̂�
	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //���_���Ƃ̃f�[�^
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
	ComPtr<ID3DBlob> vsBlob = Dx12Helper::CompileShader(L"shader/vs.hlsl", "VS", "vs_5_1");
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

	// Rasterizer
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Pixel Shader
	ComPtr<ID3DBlob> psBlob = Dx12Helper::CompileShader(L"shader/ps.hlsl", "PS", "ps_5_1");
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

	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;     //�� color : ABGR

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

