#include "PMDManager.h"

#include <unordered_map>
#include <cassert>

#include <d3dx12.h>

#include "../common.h"
#include "PMDModel.h"
#include "PMDMesh.h"
#include "VMD/VMDMotion.h"
#include "../Graphics/UploadBuffer.h"
#include "../Graphics/TextureManager.h"
#include "../Graphics/RootSignature.h"

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

	PMDObjectConstant* GetObjectConstant(const std::string& modelName);
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
	bool CreatePSO();
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
	
	ComPtr<ID3D12RootSignature> m_rootSig = nullptr;
	ComPtr<ID3D12PipelineState> m_pipeline = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS m_worldPassGpuAdress = 0;

	// Shadow depth buffer binds only to PIXEL SHADER
	// Descriptor heap stores descriptor of shadow depth buffer
	// Use for binding resource of engine to this pipeline
	ComPtr<ID3D12DescriptorHeap> m_depthHeap = nullptr;

	TextureManager m_textureMng;

private:
	std::unordered_map<std::string, PMDModel> m_loaders;
	std::vector<PMDResource> m_resources;
	std::vector<PMDRenderResource> m_renderResources;
	UploadBuffer<PMDObjectConstant> m_objectConstant;
	ComPtr<ID3D12DescriptorHeap> m_objectHeap;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_transformConstantHeapStart;
	
	std::unordered_map<std::string, uint16_t> m_modelIndices;
	uint16_t m_count = -1;
	PMDMesh m_mesh;

private:
	struct PMDAnimation
	{
		VMDMotion* pMotionData = nullptr;
		std::vector<PMDBone> Bones;
		std::unordered_map<std::string, uint16_t> BonesTable;
		float Timer = 0.0f;
		uint64_t FrameCnt = 0.0f;

		explicit PMDAnimation(std::vector<PMDBone>&& bones, 
			std::unordered_map<std::string, uint16_t>&& bonesTable) noexcept
			:Bones(bones), BonesTable(bonesTable)
		{

		}
		~PMDAnimation() { pMotionData = nullptr; };

	};
	std::unordered_map<std::string, VMDMotion> m_motionDatas;
	std::vector<DirectX::XMMATRIX> m_defaultMatrices;
	std::vector<PMDAnimation> m_animations;

	void UpdateMotionTransform(uint16_t modelIndex, const size_t& currentFrame = 0);
	void RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& matrices, size_t index);
	// Root-finding algorithm ( finding ZERO or finding ROOT )
	// There are 4 beizer points, but 2 of them are default at (0,0) and (127, 127)->(1,1) respectively
	float CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n = 8);
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
	constexpr float animation_speed = 50.0f / second_to_millisecond;
	for (const auto& data : m_modelIndices)
	{
		const auto& name = data.first;
		const auto& index = data.second;
		auto& animation = m_animations[index];
		if (animation.Timer <= 0.0f)
		{
			animation.Timer += animation_speed;
			++animation.FrameCnt;
			UpdateMotionTransform(index, animation.FrameCnt);
		}
		animation.Timer -= deltaTime;
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
	cmdList->SetGraphicsRootConstantBufferView(0, m_worldPassGpuAdress);

	// Set depth buffers
	cmdList->SetDescriptorHeaps(1, m_depthHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(1, m_depthHeap->GetGPUDescriptorHandleForHeapStart());

	// Object constant heap
	cmdList->SetDescriptorHeaps(1, m_objectHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE objectHeapHandle(m_objectHeap->GetGPUDescriptorHandleForHeapStart());
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	
	auto transformHeap = m_transformConstantHeapStart;
	for (auto& index : m_modelIndices)
	{
		auto it = m_modelIndices.begin();
		auto& name = index.first;
		auto& renderResource = m_renderResources[index.second];

		auto& startIndex = m_mesh.DrawArgs[name].StartIndexLocation;
		auto& baseVertex = m_mesh.DrawArgs[name].BaseVertexLocation;
		/*-------------Set up transform-------------*/
		
		cmdList->SetGraphicsRootDescriptorTable(2, transformHeap);
		transformHeap.Offset(1, heapSize);
		/*-------------------------------------------*/

		/*-------------Set up material-------------*/
		auto& materialHeapHandle = objectHeapHandle;
		uint32_t indexOffset = startIndex;
		for (auto& m : renderResource.SubMaterials)
		{
			cmdList->SetGraphicsRootDescriptorTable(3, materialHeapHandle);
		
			cmdList->DrawIndexedInstanced(m.indexCount,
				1,
				indexOffset,
				baseVertex,
				0);
			indexOffset += m.indexCount;
			materialHeapHandle.Offset(5, heapSize);
		}
		/*-------------------------------------------*/
	}
}

void PMDManager::Impl::DepthRender(ID3D12GraphicsCommandList* cmdList)
{
	// Set Input Assembler
	cmdList->IASetVertexBuffers(0, 1, &m_mesh.VertexBufferView);
	cmdList->IASetIndexBuffer(&m_mesh.IndexBufferView);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set world pass constant
	cmdList->SetGraphicsRootConstantBufferView(0, m_worldPassGpuAdress);

	// Object constant
	cmdList->SetDescriptorHeaps(1, m_objectHeap.GetAddressOf());
	auto transformHeap = m_transformConstantHeapStart;
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	for (auto& index : m_modelIndices)
	{
		auto& name = index.first;
		auto& heapOffset = m_renderResources[index.second].MaterialsHeapOffset;
		auto& indexCount = m_mesh.DrawArgs[name].IndexCount;
		auto& startIndex = m_mesh.DrawArgs[name].StartIndexLocation;
		auto& baseVertex = m_mesh.DrawArgs[name].BaseVertexLocation;
		
		cmdList->SetGraphicsRootDescriptorTable(1, transformHeap);
		transformHeap.Offset(1, heapSize);
		cmdList->DrawIndexedInstanced(indexCount, 1, startIndex, baseVertex, 0);
	}
}

bool PMDManager::Impl::CreatePipeline()
{
	if (!CreateRootSignature()) return false;
	if (!CreatePSO()) return false;
	return true;
}

bool PMDManager::Impl::CreateRootSignature()
{
	RootSignature rootSignature;

	// World pass constant
	rootSignature.AddRootParameterAsRootDescriptor(RootSignature::CONSTANT_BUFFER_VIEW);
	// Depth
	rootSignature.AddRootParameterAsDescriptorTable(0, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	// Object Constant
	rootSignature.AddRootParameterAsDescriptorTable(1, 0, 0);
	// Material Constant
	rootSignature.AddRootParameterAsDescriptorTable(1, 4, 0, D3D12_SHADER_VISIBILITY_PIXEL);
	rootSignature.AddStaticSampler();
	rootSignature.Init(m_device);

	m_rootSig = rootSignature.Get();

	return true;
}

bool PMDManager::Impl::CreatePSO()
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
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG" , "1",
		nullptr, nullptr
	};
	ComPtr<ID3DBlob> psBlob = D12Helper::CompileShaderFromFile(L"Shader/ps.hlsl", "PS", "ps_5_1", defines);
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

void PMDManager::Impl::UpdateMotionTransform(uint16_t modelIndex, const size_t& currentFrame)
{
	// If model don't have animtion, don't need to do motion
	if (!m_animations[modelIndex].pMotionData) return;

	auto& animation = m_animations[modelIndex];
	auto& resource = m_resources[modelIndex];
	auto& motionData = animation.pMotionData->GetVMDMotionData();
	auto mats = m_defaultMatrices;

	for (auto& motion : motionData)
	{
		auto index = animation.BonesTable[motion.first];
		auto& rotationOrigin = animation.Bones[index].pos;
		auto& keyframe = motion.second;

		auto rit = std::find_if(keyframe.rbegin(), keyframe.rend(),
			[currentFrame](const VMDData& it)
			{
				return it.frameNO <= currentFrame;
			});
		auto it = rit.base();

		if (rit == keyframe.rend()) continue;

		float t = 0.0f;
		auto q = XMLoadFloat4(&rit->quaternion);
		auto move = rit->location;

		if (it != keyframe.end())
		{
			t = static_cast<float>(currentFrame - rit->frameNO) / static_cast<float>(it->frameNO - rit->frameNO);
			t = CalculateFromBezierByHalfSolve(t, it->b1, it->b2);
			q = XMQuaternionSlerp(q, XMLoadFloat4(&it->quaternion), t);
			XMStoreFloat3(&move, XMVectorLerp(XMLoadFloat3(&move), XMLoadFloat3(&it->location), t));
		}
		mats[index] *= XMMatrixTranslation(-rotationOrigin.x, -rotationOrigin.y, -rotationOrigin.z);
		mats[index] *= XMMatrixRotationQuaternion(q);
		mats[index] *= XMMatrixTranslation(rotationOrigin.x, rotationOrigin.y, rotationOrigin.z);
		mats[index] *= XMMatrixTranslation(move.x, move.y, move.z);
	}

	RecursiveCalculate(animation.Bones, mats, 0);
	auto mappedBones = m_objectConstant.GetHandleMappedData(modelIndex);
	std::copy(mats.begin(), mats.end(), mappedBones->bones);
}

void PMDManager::Impl::RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& matrices, size_t index)
// bones : bones' data
// matrices : matrices use for bone's Transformation
{
	auto& bonePos = bones[index].pos;
	auto& mat = matrices[index];    // parent's matrix 

	for (auto& childIndex : bones[index].children)
	{
		matrices[childIndex] *= mat;
		RecursiveCalculate(bones, matrices, childIndex);
	}
}

float PMDManager::Impl::CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n)
{
	// (y = x) is a straight line -> do not need to calculate
	if (p1.x == p1.y && p2.x == p2.y)
		return x;
	// Bezier method
	float t = x;
	float k0 = 3 * p1.x - 3 * p2.x + 1;         // t^3
	float k1 = -6 * p1.x + 3 * p2.x;            // t^2
	float k2 = 3 * p1.x;                        // t

	constexpr float eplison = 0.00005f;
	for (int i = 0; i < n; ++i)
	{
		// f(t) = t*t*t*k0 + t*t*k1 + t*k2
		// f = f(t) - x
		// process [f(t) - x] to reach approximate 0
		// => f -> ~0
		// => |f| = ~eplison
		auto f = t * t * t * k0 + t * t * k1 + t * k2 - x;
		if (f >= -eplison || f <= eplison) break;
		t -= f / 2;
	}

	auto rt = 1 - t;
	// y = f(t)
	// t = g(x)
	// -> y = f(g(x))
	return t * t * t + 3 * (rt * rt) * t * p1.y + 3 * rt * (t * t) * p2.y;
}


bool PMDManager::Impl::Init(ID3D12GraphicsCommandList* cmdList)
{
	if (m_isInitDone) return true;
	if (!m_device) return false;
	if (!m_worldPassGpuAdress) return false;
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
	const uint16_t model_count = m_loaders.size();

	uint16_t descriptor_count = 0;
	uint16_t materials_descriptor_count = 0;
	// number of material's descriptors of all models
	for (auto& model : m_loaders)
	{
		materials_descriptor_count += model.second.MaterialDescriptorCount;
	}
	// number of object constant's descriptors of all models
	descriptor_count = materials_descriptor_count + model_count;

	// Create object constant heap
	D12Helper::CreateDescriptorHeap(m_device, m_objectHeap, descriptor_count,
		D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_objectHeap->GetCPUDescriptorHandleForHeapStart());
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_transformConstantHeapStart.Offset(materials_descriptor_count, heapSize);

	for (auto& model : m_loaders)
	{
		model.second.SetDefaultTexture(m_whiteTexture, m_blackTexture, m_gradTexture);
		model.second.CreateModel(cmdList, heapHandle);
	}

	// Load model datas to Manager's resources
	uint16_t index = 0;
	m_resources.reserve(model_count);
	m_renderResources.reserve(model_count);
	for (auto& model : m_loaders)
	{
		auto& name = model.first;
		auto& data = model.second;
		m_resources.push_back(std::move(data.Resource));
		m_renderResources.push_back(std::move(data.RenderResource));
		++index;
	}
	
	// Create default bones matrices
	m_defaultMatrices.reserve(512);
	for (uint16_t i = 0; i < 512; ++i)
	{
		m_defaultMatrices.push_back(XMMatrixIdentity());
	}

	// Create object constant
	m_objectConstant.Create(m_device, model_count, true);
	for (uint16_t i = 0; i < model_count; ++i)
	{
		auto hMappedData = m_objectConstant.GetHandleMappedData(i);
		hMappedData->world = XMMatrixIdentity();
		hMappedData->texTransform = XMMatrixIdentity();
		std::copy(m_defaultMatrices.begin(), m_defaultMatrices.end(), hMappedData->bones);
	}

	// Create object constant view
	auto objectConstantAddress = m_objectConstant.GetGPUVirtualAddress();
	const auto object_constant_element_size = m_objectConstant.ElementSize();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = object_constant_element_size;
	for (uint16_t i = 0; i < model_count; ++i)
	{
		cbvDesc.BufferLocation = objectConstantAddress;
		
		m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

		heapHandle.Offset(1, heapSize);
		objectConstantAddress += object_constant_element_size;
	}

	// Init model animation
	m_animations.reserve(model_count);
	for (auto& model : m_loaders)
	{
		auto& name = model.first;
		auto& data = model.second;
		m_animations.emplace_back(std::move(data.Bones), std::move(data.BonesTable));
	}

	uint32_t indexCount = 0;
	uint32_t vertexCount = 0;
	// Loop for calculate size of indices of all models
	// And save baseIndex of each model for Render Usage
	for (auto& model : m_loaders)
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
	for (auto& model : m_loaders)
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

	m_loaders.clear();
}

bool PMDManager::Impl::HasModel(std::string const& modelName)
{
	return m_modelIndices.count(modelName);
}

bool PMDManager::Impl::HasAnimation(std::string const& animationName)
{
	return m_motionDatas.count(animationName);
}

bool PMDManager::Impl::ClearSubresource()
{
	m_mesh.ClearSubresource();
	for (auto& model : m_loaders)
		model.second.ClearSubresources();
	return true;
}

PMDObjectConstant* PMDManager::Impl::GetObjectConstant(const std::string& modelName)
{
	return m_objectConstant.GetHandleMappedData(m_modelIndices[modelName]);
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

bool PMDManager::SetWorldPassConstantGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS worldPassConstantGpuAddress)
{
	assert(IMPL.m_device);
	if (IMPL.m_device == nullptr) return false;
	if (worldPassConstantGpuAddress <= 0) return false;

	IMPL.m_worldPassGpuAdress = worldPassConstantGpuAddress;

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
	IMPL.m_loaders[modelName].SetDevice(IMPL.m_device);
	IMPL.m_loaders[modelName].Load(modelFilePath);
	IMPL.m_modelIndices[modelName] = ++IMPL.m_count;
	return true;
}

bool PMDManager::CreateAnimation(const std::string& animationName, const char* animationFilePath)
{
	assert(!IMPL.HasAnimation(animationName));
	if (IMPL.HasAnimation(animationName)) return false;
	IMPL.m_motionDatas[animationName].Load(animationFilePath);
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

	IMPL.m_animations[IMPL.m_modelIndices[modelName]].pMotionData = &IMPL.m_motionDatas[animationName];

	return true;
}

bool PMDManager::Move(const std::string& modelName, float moveX, float moveY, float moveZ)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	auto& world = IMPL.GetObjectConstant(modelName)->world;
	world *= XMMatrixTranslation(moveX, moveY, moveZ);
	return true;
}

bool PMDManager::RotateX(const std::string& modelName, float angle)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	auto& world = IMPL.GetObjectConstant(modelName)->world;
	world *= XMMatrixRotationX(angle);
	return true;
}

bool PMDManager::RotateY(const std::string& modelName, float angle)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	auto& world = IMPL.GetObjectConstant(modelName)->world;
	world *= XMMatrixRotationY(angle);
	return true;
}

bool PMDManager::RotateZ(const std::string& modelName, float angle)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	auto& world = IMPL.GetObjectConstant(modelName)->world;
	world *= XMMatrixRotationZ(angle);
	return true;
}

bool PMDManager::Scale(const std::string& modelName, float scaleX, float scaleY, float scaleZ)
{
	assert(IMPL.HasModel(modelName));
	if (!IMPL.HasModel(modelName)) return false;
	auto& world = IMPL.GetObjectConstant(modelName)->world;
	world *= XMMatrixScaling(scaleX, scaleY, scaleZ);
	return true;
}

