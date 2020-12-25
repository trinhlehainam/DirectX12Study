#include "PMDManager.h"
#include "PMDModel.h"
#include "VMD/VMDMotion.h"
#include "PMDMesh.h"

#include <unordered_map>
#include <cassert>
#include <d3dx12.h>

#define IMPL (*m_impl)

class PMDManager::Impl
{
public:
	Impl();
	explicit Impl(ID3D12Device* pDevice);
	~Impl();
private:
	friend PMDManager;

	bool Init(ID3D12GraphicsCommandList* cmdList);

	void InitModels(ID3D12GraphicsCommandList* cmdList);
	bool HasModel(std::string const& modelName);
	bool HasAnimation(std::string const& animationName);
	bool ClearSubresource();
private:
	void Update(const float& deltaTime);
	void Render(ID3D12GraphicsCommandList* cmdList);
	void RenderDepth(ID3D12GraphicsCommandList* cmdList);

private:
	// When Initialization hasn't done
	// =>Client's Render and Update methods are putted to sleep
	void SleepUpdate(const float& deltaTime);
	void SleepRender(ID3D12GraphicsCommandList* cmdList);

	void NormalUpdate(const float& deltaTime);
	void NormalRender(ID3D12GraphicsCommandList* cmdList);

	// Function use for taking models depth value
	void DepthRender(ID3D12GraphicsCommandList* cmdList);

	using UpdateFunc_ptr = void (PMDManager::Impl::*)(const float& deltaTime);
	using RenderFunc_ptr = void (PMDManager::Impl::*)(ID3D12GraphicsCommandList* pGraphicsCmdList);
	UpdateFunc_ptr m_updateFunc;
	RenderFunc_ptr m_renderFunc;
	RenderFunc_ptr m_renderDepthFunc;

	bool m_isInitDone = false;
private:

	bool CreatePipeline();
	bool CreateRootSignature();
	bool CreatePipelineStateObject();
	// check default buffers are initialized or not
	bool CheckDefaultBuffers();

	/*----------RESOURCE FROM ENGINE----------*/
	// Device from engine
	ID3D12Device* m_device = nullptr;

	// Default resources from engine
	ID3D12Resource* m_whiteTexture = nullptr;
	ID3D12Resource* m_blackTexture = nullptr;
	ID3D12Resource* m_gradTexture = nullptr;
	/*-----------------------------------------*/

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline = nullptr;

	// World pass constant buffer binds only to VERTEX BUFFER
	// Descriptor heap stores descriptor of world pass constant buffer
	// Use for binding resource of engine to this pipeline
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_worldPCBHeap = nullptr;

	// Shadow depth buffer binds only to PIXEL SHADER
	// Descriptor heap stores descriptor of shadow depth buffer
	// Use for binding resource of engine to this pipeline
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_depthHeap = nullptr;

private:
	std::unordered_map<std::string, PMDModel> m_models;
	std::unordered_map<std::string, VMDMotion> m_animations;
	PMDMesh m_mesh;
};

PMDManager::Impl::Impl()
{
	m_updateFunc = &PMDManager::Impl::SleepUpdate;
	m_renderFunc = &PMDManager::Impl::SleepRender;
	m_renderDepthFunc = &PMDManager::Impl::SleepRender;
}

PMDManager::Impl::Impl(ID3D12Device* pDevice) :m_device(pDevice)
{
	m_updateFunc = &PMDManager::Impl::SleepUpdate;
	m_renderFunc = &PMDManager::Impl::SleepRender;
	m_renderDepthFunc = &PMDManager::Impl::SleepRender;
}

PMDManager::Impl::~Impl()
{
	m_device = nullptr;
	m_whiteTexture = nullptr;
	m_blackTexture = nullptr;
	m_gradTexture = nullptr;
}

void PMDManager::Impl::Update(const float& deltaTime)
{
	(this->*m_updateFunc)(deltaTime);
}

void PMDManager::Impl::Render(ID3D12GraphicsCommandList* cmdList)
{
	(this->*m_renderFunc)(cmdList);
}

void PMDManager::Impl::RenderDepth(ID3D12GraphicsCommandList* cmdList)
{
	(this->*m_renderDepthFunc)(cmdList);
}

void PMDManager::Impl::SleepUpdate(const float& deltaTime)
{
	assert(m_isInitDone);
}

void PMDManager::Impl::SleepRender(ID3D12GraphicsCommandList* cmdList)
{
	assert(m_isInitDone);
}

void PMDManager::Impl::NormalUpdate(const float& deltaTime)
{
	for (auto& model : m_models)
	{
		model.second.Update(deltaTime);
	}
}

void PMDManager::Impl::NormalRender(ID3D12GraphicsCommandList* cmdList)
{
	cmdList->SetPipelineState(m_pipeline.Get());
	cmdList->SetGraphicsRootSignature(m_rootSig.Get());

	// Set Input Assembler
	cmdList->IASetVertexBuffers(0, 1, &m_mesh.VertexBufferView);
	cmdList->IASetIndexBuffer(&m_mesh.IndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set world pass constant
	cmdList->SetDescriptorHeaps(1, m_worldPCBHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(0, m_worldPCBHeap->GetGPUDescriptorHandleForHeapStart());

	// Set depth buffers
	cmdList->SetDescriptorHeaps(1, m_depthHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(1, m_depthHeap->GetGPUDescriptorHandleForHeapStart());

	// Render all models
	for (auto& model : m_models)
	{
		auto& name = model.first;
		auto& data = model.second;

		auto& startIndex = m_mesh.DrawArgs[name].StartIndexLocation;
		auto& baseVertex = m_mesh.DrawArgs[name].BaseVertexLocation;
		data.Render(cmdList, startIndex, baseVertex);
	}
}

void PMDManager::Impl::DepthRender(ID3D12GraphicsCommandList* cmdList)
{
	// Set Input Assembler
	cmdList->IASetVertexBuffers(0, 1, &m_mesh.VertexBufferView);
	cmdList->IASetIndexBuffer(&m_mesh.IndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set world pass constant
	cmdList->SetDescriptorHeaps(1, m_worldPCBHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(0, m_worldPCBHeap->GetGPUDescriptorHandleForHeapStart());

	// Render all models
	for (auto& model : m_models)
	{
		auto& name = model.first;
		auto& data = model.second;
		auto& indexCount = m_mesh.DrawArgs[name].IndexCount;
		auto& startIndex = m_mesh.DrawArgs[name].StartIndexLocation;
		auto& baseVertex = m_mesh.DrawArgs[name].BaseVertexLocation;

		data.RenderDepth(cmdList, indexCount, startIndex, baseVertex);
	}
}

bool PMDManager::Impl::CreatePipeline()
{
	if (!CreateRootSignature()) return false;
	if (!CreatePipelineStateObject()) return false;
	return true;
}

bool PMDManager::Impl::CreateRootSignature()
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

	// Depth
	range[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
	parameter[1].InitAsDescriptorTable(1, &range[1], D3D12_SHADER_VISIBILITY_PIXEL);

	// Object Constant
	range[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
	parameter[2].InitAsDescriptorTable(1, &range[2], D3D12_SHADER_VISIBILITY_VERTEX);

	// Material
	range[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
	range[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 2);
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
	D12Helper::ThrowIfFailed(D3D12SerializeRootSignature(&rtSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
		&rootSigBlob,
		&errBlob));
	D12Helper::OutputFromErrorBlob(errBlob);
	D12Helper::ThrowIfFailed(m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(),
		rootSigBlob->GetBufferSize(), IID_PPV_ARGS(m_rootSig.ReleaseAndGetAddressOf())));

	return true;
}

bool PMDManager::Impl::CreatePipelineStateObject()
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
	ComPtr<ID3DBlob> vsBlob = D12Helper::CompileShaderFromFile(L"Shader/vs.hlsl", "VS", "vs_5_1");
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
	// Rasterizer
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	// Pixel Shader
	ComPtr<ID3DBlob> psBlob = D12Helper::CompileShaderFromFile(L"Shader/ps.hlsl", "PS", "ps_5_1");
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
	psoDesc.NumRenderTargets = 2;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;			// main render target
	psoDesc.RTVFormats[1] = DXGI_FORMAT_R8G8B8A8_UNORM;			// normal render target
	// Blend
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// Root Signature
	psoDesc.pRootSignature = m_rootSig.Get();
	D12Helper::ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc,
		IID_PPV_ARGS(m_pipeline.GetAddressOf())));

	return true;
}

bool PMDManager::Impl::CheckDefaultBuffers()
{
	return m_whiteTexture && m_blackTexture && m_gradTexture;
}


//
/***************** PMDManager public method *******************/
//

PMDManager::PMDManager():m_impl(new Impl())
{
	
}

PMDManager::PMDManager(ID3D12Device* pDevice): m_impl(new Impl(pDevice))
{
	
}

PMDManager::PMDManager(const PMDManager&)
{

}

void PMDManager::operator=(const PMDManager&)
{
}

PMDManager::~PMDManager()
{
	SAFE_DELETE(m_impl);
}

bool PMDManager::SetDefaultBuffer(ID3D12Resource* pWhiteTexture, ID3D12Resource* pBlackTexture, ID3D12Resource* pGradTexture)
{
	if (!pWhiteTexture || !pBlackTexture || !pGradTexture) return false;

	IMPL.m_whiteTexture = pWhiteTexture;
	IMPL.m_blackTexture = pBlackTexture;
	IMPL.m_gradTexture = pGradTexture;

	return true;
}

bool PMDManager::Init(ID3D12GraphicsCommandList* cmdList)
{
	return IMPL.Init(cmdList);
}

bool PMDManager::ClearSubresources()
{
	return IMPL.ClearSubresource();
}

bool PMDManager::SetDevice(ID3D12Device* pDevice)
{
    if (pDevice == nullptr) return false;
	IMPL.m_device = pDevice;
    return false;
}

bool PMDManager::SetWorldPassConstant(ID3D12Resource* pWorldPassConstant , size_t bufferSize)
{
	assert(IMPL.m_device);
	if (IMPL.m_device == nullptr) return false;
	if (pWorldPassConstant == nullptr) return false;
	if (bufferSize == 0) return false;

	D12Helper::CreateDescriptorHeap(IMPL.m_device, IMPL.m_worldPCBHeap,
		1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = pWorldPassConstant->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = bufferSize;

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(IMPL.m_worldPCBHeap->GetCPUDescriptorHandleForHeapStart());
	IMPL.m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

	return true;
}

bool PMDManager::SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer)
{
	assert(IMPL.m_device);
	if (pShadowDepthBuffer == nullptr) return false;

	D12Helper::CreateDescriptorHeap(IMPL.m_device, m_impl->m_depthHeap, 2,
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

bool PMDManager::SetViewDepth(ID3D12Resource* pViewDepthBuffer)
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

bool PMDManager::IsInitialized()
{
	return IMPL.m_isInitDone;
}

bool PMDManager::CreateModel(const std::string& modelName, const char* modelFilePath)
{
	assert(!IMPL.HasModel(modelName));
	if (IMPL.HasModel(modelName)) return false;
	IMPL.m_models[modelName].SetDevice(IMPL.m_device);
	IMPL.m_models[modelName].Load(modelFilePath);
	return true;
}

bool PMDManager::CreateAnimation(const std::string& animationName, const char* animationFilePath)
{
	assert(!IMPL.HasAnimation(animationName));
	if (IMPL.HasAnimation(animationName)) return false;
	IMPL.m_animations[animationName].Load(animationFilePath);
	return true;
}

void PMDManager::Update(const float& deltaTime)
{
	IMPL.Update(deltaTime);
}

void PMDManager::Render(ID3D12GraphicsCommandList* cmdList)
{
	IMPL.Render(cmdList);
}

void PMDManager::RenderDepth(ID3D12GraphicsCommandList* cmdList)
{
	IMPL.RenderDepth(cmdList);
}

bool PMDManager::Play(const std::string& modelName, const std::string& animationName)
{
	if (!IMPL.m_isInitDone) return false;
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	assert(IMPL.HasAnimation(animationName));
	if (!IMPL.HasAnimation(animationName)) return false;

	IMPL.m_models[modelName].Play(&IMPL.m_animations[animationName]);

	return true;
}

bool PMDManager::Move(const std::string& modelName, float moveX, float moveY, float moveZ)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	IMPL.m_models[modelName].Move(moveX, moveY, moveZ);
	return true;
}

bool PMDManager::RotateX(const std::string& modelName, float angle)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	IMPL.m_models[modelName].RotateX(angle);
	return true;
}

bool PMDManager::RotateY(const std::string& modelName, float angle)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	IMPL.m_models[modelName].RotateY(angle);
	return true;
}

bool PMDManager::RotateZ(const std::string& modelName, float angle)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	IMPL.m_models[modelName].RotateZ(angle);
	return true;
}

bool PMDManager::Scale(const std::string& modelName, float scaleX, float scaleY, float scaleZ)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	IMPL.m_models[modelName].Scale(scaleX, scaleY, scaleZ);
	return true;
}

bool PMDManager::Impl::Init(ID3D12GraphicsCommandList* cmdList)
{
	if (m_isInitDone) return true;
	if (!m_device) return false;
	if (!m_worldPCBHeap) return false;
	if (!m_depthHeap) return false;

	if (!CheckDefaultBuffers()) return false;
	if (!CreatePipeline()) return false;

	InitModels(cmdList);

	m_updateFunc = &PMDManager::Impl::NormalUpdate;
	m_renderFunc = &PMDManager::Impl::NormalRender;
	m_renderDepthFunc = &PMDManager::Impl::DepthRender;
	m_isInitDone = true;

	return true;
}

void PMDManager::Impl::InitModels(ID3D12GraphicsCommandList* cmdList)
{
	// Init all models
	for (auto& model : m_models)
	{
		model.second.SetDefaultTexture(m_whiteTexture, m_blackTexture, m_gradTexture);
		model.second.CreateModel(cmdList);
	}

	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	// Loop for calculate size of indices of all models
	// And save baseIndex of each model for Render Usage
	for (auto& model : m_models)
	{
		auto& data = model.second;
		auto& name = model.first;

		m_mesh.DrawArgs[name].StartIndexLocation = indexCount;
		m_mesh.DrawArgs[name].BaseVertexLocation = vertexCount;
		m_mesh.DrawArgs[name].IndexCount = data.Indices().size();
		indexCount += data.Indices().size();
		vertexCount += data.Vertices().size();
	}

	m_mesh.Indices16.reserve(indexCount);
	m_mesh.Vertices.reserve(vertexCount);

	// Add all vertices and indices to PMDMeshes
	for (auto& model : m_models)
	{
		auto& name = model.first;;
		auto& data = model.second;

		for (const auto& index : data.Indices())
			m_mesh.Indices16.push_back(index);

		for (const auto& vertex : data.Vertices())
			m_mesh.Vertices.push_back(vertex);
	}

	m_mesh.CreateBuffers(m_device, cmdList);
	m_mesh.CreateViews();
}

bool PMDManager::Impl::HasModel(std::string const& modelName)
{
	return m_models.count(modelName);
}

bool PMDManager::Impl::HasAnimation(std::string const& animationName)
{
	return m_animations.count(animationName);
}

bool PMDManager::Impl::ClearSubresource()
{
	m_mesh.ClearSubresource();
	for (auto& model : m_models)
		model.second.ClearSubresources();
	return true;
}
