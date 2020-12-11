#include "PMDModel.h"
#include "../Application.h"
#include "../VMDLoader/VMDMotion.h"
#include "../DirectX12/Dx12Helper.h"
#include "../Common/StringHelper.h"

#include <stdio.h>
#include <Windows.h>
#include <array>
#include <d3dcompiler.h>

namespace
{
	const char* pmd_path = "resource/PMD/";
	constexpr unsigned int material_descriptor_count = 5;
}


void PMDModel::CreateModel()
{
	CreateTexture();
	CreateModelPipeline();
}

void PMDModel::Transform(const DirectX::XMMATRIX& transformMatrix)
{
	mappedBasicMatrix_->world *= transformMatrix;
}

uint16_t PMDModel::GetIndicesSize() const
{
	return indices_.size();
}

void PMDModel::LoadMotion(const char* path)
{
	vmdMotion_->Load(path);
}

void PMDModel::Render(ComPtr<ID3D12GraphicsCommandList>& cmdList, const size_t& frameNO)
{
	auto frame = frameNO % vmdMotion_->GetMaxFrame();
	UpdateMotionTransform(frame);

	SetGraphicPinelineState(cmdList);
	// Set Input Assemble
	SetInputAssembler(cmdList);

	/*-------------Set up transform-------------*/
	SetTransformGraphicPipeline(cmdList);
	/*-------------------------------------------*/

	cmdList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)shadowDepthHeap_.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(2, shadowDepthHeap_->GetGPUDescriptorHandleForHeapStart());

	/*-------------Set up material and texture-------------*/
	cmdList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)materialDescHeap_.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE materialHeapHandle(materialDescHeap_->GetGPUDescriptorHandleForHeapStart());
	auto materialHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32_t indexOffset = 0;
	for (auto& m : materials_)
	{
		cmdList->SetGraphicsRootDescriptorTable(1, materialHeapHandle);

		cmdList->DrawIndexedInstanced(m.indices,
			1,
			indexOffset,
			0,
			0);
		indexOffset += m.indices;
		materialHeapHandle.Offset(material_descriptor_count, materialHeapSize);
	}
	/*-------------------------------------------*/
}

void PMDModel::SetInputAssembler(ComPtr<ID3D12GraphicsCommandList>& cmdList)
{
	cmdList->IASetVertexBuffers(0, 1, &vbView_);
	cmdList->IASetIndexBuffer(&ibView_);
	cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

ComPtr<ID3D12Resource> PMDModel::GetTransformBuffer()
{
	return transformBuffer_;
}

void PMDModel::SetTransformGraphicPipeline(ComPtr<ID3D12GraphicsCommandList>& cmdList)
{
	cmdList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)transformDescHeap_.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(0, transformDescHeap_->GetGPUDescriptorHandleForHeapStart());
}

void PMDModel::SetGraphicPinelineState(ComPtr<ID3D12GraphicsCommandList>& cmdList)
{
	cmdList->SetPipelineState(pipeline_.Get());
	cmdList->SetGraphicsRootSignature(rootSig_.Get());
}

WorldPassConstant* PMDModel::GetMappedMatrix()
{
	return mappedBasicMatrix_;
}

void PMDModel::GetDefaultTexture(ComPtr<ID3D12Resource>& whiteTexture, ComPtr<ID3D12Resource>& blackTexture, ComPtr<ID3D12Resource>& gradTexture)
{
	whiteTexture_ = whiteTexture;
	blackTexture_ = blackTexture;
	gradTexture_ = gradTexture;
}

PMDModel::PMDModel(ComPtr<ID3D12Device> device) :
	m_device(device)
{
	vmdMotion_ = std::make_shared<VMDMotion>();
}

PMDModel::~PMDModel()
{
	boneBuffer_->Unmap(0, nullptr);
	vertexBuffer_->Unmap(0, nullptr);
	m_device = nullptr;
}

bool PMDModel::LoadPMD(const char* path)
{
	pmdPath_ = path;

	//識別子"pmd"
	FILE* fp = nullptr;
	auto err = fopen_s(&fp, path, "rb");
	if (fp == nullptr || err != 0)
	{
		char cerr[256];
		strerror_s(cerr, _countof(cerr), err);
		OutputDebugStringA(cerr);
	}
#pragma pack(1)
	struct PMDHeader {
		char id[3];
		// padding
		float version;
		char name[20];
		char comment[256];
	};

	struct Vertex
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal_vec;
		DirectX::XMFLOAT2 uv;
		WORD bone_num[2];
		// 36 bytes
		BYTE bone_weight;
		BYTE edge_flag;
		// 38 bytes
		// padding 2 bytes
	};

	struct Material
	{
		DirectX::XMFLOAT3 diffuse;
		FLOAT alpha;
		FLOAT specularity;
		DirectX::XMFLOAT3 specular_color;
		DirectX::XMFLOAT3 mirror_color;
		BYTE toon_index;
		BYTE edge_flag;
		DWORD face_vert_count;
		char textureFileName[20];
	};

	struct BoneData
	{
		char boneName[20];
		uint16_t parentNo;
		uint16_t tailNo;
		uint8_t type;
		uint16_t ikParentNo;
		DirectX::XMFLOAT3 pos;
	}; // 39 bytes
#pragma pack()

	PMDHeader header;
	fread_s(&header, sizeof(header), sizeof(header), 1, fp);
	uint32_t cVertex = 0;
	fread_s(&cVertex, sizeof(cVertex), sizeof(cVertex), 1, fp);
	std::vector<Vertex> vertices(cVertex);
	fread_s(vertices.data(), vertices.size() * sizeof(Vertex), vertices.size() *  sizeof(Vertex), 1, fp);
	vertices_.resize(cVertex);
	for (int i = 0; i < cVertex; ++i)
	{
		vertices_[i].pos = vertices[i].pos;
		vertices_[i].normal = vertices[i].normal_vec;
		vertices_[i].uv = vertices[i].uv;
		std::copy(std::begin(vertices[i].bone_num),
			std::end(vertices[i].bone_num), vertices_[i].boneNo);
		vertices_[i].weight = static_cast<float>(vertices[i].bone_weight) / 100.0f;
	}
	uint32_t cIndex = 0;
	fread_s(&cIndex, sizeof(cIndex), sizeof(cIndex), 1, fp);
	indices_.resize(cIndex);
	fread_s(indices_.data(), sizeof(indices_[0])* indices_.size(), sizeof(indices_[0])* indices_.size(), 1, fp);
	uint32_t cMaterial = 0;
	fread_s(&cMaterial, sizeof(cMaterial), sizeof(cMaterial), 1, fp);
	std::vector<Material> materials(cMaterial);
	fread_s(materials.data(), sizeof(materials[0])* materials.size(), sizeof(materials[0])* materials.size(), 1, fp);

	uint16_t boneNum = 0;
	fread_s(&boneNum, sizeof(boneNum), sizeof(boneNum), 1, fp);

	// Bone
	std::vector<BoneData> boneData(boneNum);
	fread_s(boneData.data(), sizeof(boneData[0])* boneData.size(), sizeof(boneData[0])* boneData.size(), 1, fp);
	m_boneMatrices.resize(boneNum);
	for (int i = 0; i < boneNum; ++i)
	{
		m_boneMatrices[i].name = boneData[i].boneName;
		m_boneMatrices[i].pos = boneData[i].pos;
		bonesTable_[boneData[i].boneName] = i;
	}
	boneMatrices_.resize(boneNum);
	std::fill(boneMatrices_.begin(), boneMatrices_.end(), DirectX::XMMatrixIdentity());

	for (int i = 0; i < boneNum; ++i)
	{
		if (boneData[i].parentNo == 0xffff) continue;
		auto pno = boneData[i].parentNo;
		m_boneMatrices[pno].children.push_back(i);
#ifdef _DEBUG
		m_boneMatrices[pno].childrenName.push_back(boneData[i].boneName);
#endif
	}

	// IK(inverse kematic)
	uint16_t ikNum = 0;
	fread_s(&ikNum, sizeof(ikNum), sizeof(ikNum), 1, fp);
	for (int i = 0; i < ikNum; ++i)
	{
		fseek(fp, 4, SEEK_CUR);
		uint8_t chainNum;
		fread_s(&chainNum, sizeof(chainNum), sizeof(chainNum), 1, fp);
		fseek(fp, sizeof(uint16_t) + sizeof(float), SEEK_CUR);
		fseek(fp, sizeof(uint16_t) * chainNum, SEEK_CUR);
	}

	uint16_t skinNum = 0;
	fread_s(&skinNum, sizeof(skinNum), sizeof(skinNum), 1, fp);
	for (int i = 0; i < skinNum; ++i)
	{
		fseek(fp, 20, SEEK_CUR);	// Name of facial skin
		uint32_t skinVertCnt = 0;		// number of facial vertex
		fread_s(&skinVertCnt, sizeof(skinVertCnt), sizeof(skinVertCnt), 1, fp);
		fseek(fp, 1, SEEK_CUR);		// kind of facial
		fseek(fp, 16 * skinVertCnt, SEEK_CUR); // position of vertices
	}

	uint8_t skinDispNum = 0;
	fread_s(&skinDispNum, sizeof(skinDispNum), sizeof(skinDispNum), 1, fp);
	fseek(fp, sizeof(uint16_t)* skinDispNum, SEEK_CUR);

	uint8_t ikNameNum = 0;
	fread_s(&ikNameNum, sizeof(ikNameNum), sizeof(ikNameNum), 1, fp);
	fseek(fp, 50 * ikNameNum, SEEK_CUR);

	uint32_t boneDispNum = 0;
	fread_s(&boneDispNum, sizeof(boneDispNum), sizeof(boneDispNum), 1, fp);
	fseek(fp, 
		(sizeof(uint16_t) + sizeof(uint8_t)) * boneDispNum, 
		SEEK_CUR);

	uint8_t isEngAvalable = 0;
	fread_s(&isEngAvalable, sizeof(isEngAvalable), sizeof(isEngAvalable), 1, fp);
	if (isEngAvalable)
	{
		fseek(fp, 276, SEEK_CUR);
		fseek(fp, boneNum * 20, SEEK_CUR);
		fseek(fp, (skinNum-1) * 20, SEEK_CUR);		// list of facial skin's English name
		fseek(fp, ikNameNum * 50, SEEK_CUR);
	}
	
	std::array<char[100], 10> toonNames;
	fread_s(toonNames.data(), sizeof(toonNames[0])* toonNames.size(), sizeof(toonNames[0])* toonNames.size(), 1, fp);

	for (auto& m : materials)
	{
		if(m.toon_index > toonNames.size() - 1)
			toonPaths_.push_back(toonNames[0]);
		else
			toonPaths_.push_back(toonNames[m.toon_index]);

		modelPaths_.push_back(m.textureFileName);
		materials_.push_back({ m.diffuse,m.alpha,m.specular_color,m.specularity,m.mirror_color,m.face_vert_count });
	}

	fclose(fp);

	return true;
}

void PMDModel::CreateVertexBuffer()
{
	HRESULT result = S_OK;

	vertexBuffer_ = Dx12Helper::CreateBuffer(m_device, sizeof(vertices_[0]) * vertices_.size());

	result = vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertex_));
	std::copy(std::begin(vertices_), std::end(vertices_), mappedVertex_);
	assert(SUCCEEDED(result));

	vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
	vbView_.SizeInBytes = sizeof(vertices_[0]) * vertices_.size();
	vbView_.StrideInBytes = sizeof(vertices_[0]);
}

void PMDModel::CreateIndexBuffer()
{
	HRESULT result = S_OK;

	indicesBuffer_ = Dx12Helper::CreateBuffer(m_device, sizeof(indices_[0]) * indices_.size());

	ibView_.BufferLocation = indicesBuffer_->GetGPUVirtualAddress();
	ibView_.SizeInBytes = sizeof(indices_[0]) * indices_.size();
	ibView_.Format = DXGI_FORMAT_R16_UINT;

	uint16_t* mappedIdxData = nullptr;
	result = indicesBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIdxData));
	std::copy(std::begin(indices_), std::end(indices_), mappedIdxData);
	assert(SUCCEEDED(result));
	indicesBuffer_->Unmap(0, nullptr);
}

bool PMDModel::CreateTransformBuffer()
{
	HRESULT result = S_OK;

	Dx12Helper::CreateDescriptorHeap(m_device.Get(), transformDescHeap_, 
		2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	transformBuffer_ = Dx12Helper::CreateBuffer(m_device, Dx12Helper::AlignedConstantBufferMemory(sizeof(WorldPassConstant)));

	auto wSize = Application::Instance().GetWindowSize();
	XMMATRIX tempMat = XMMatrixIdentity();

	// world coordinate
	auto angle = 4 * XM_PI / 3.0f;
	XMMATRIX world = XMMatrixRotationY(angle);

	// camera array (view)
	XMMATRIX viewproj = XMMatrixLookAtRH(
		{ 10.0f, 10.0f, 10.0f, 1.0f },
		{ 0.0f, 10.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 0.0f, 0.0f });

	// projection array
	viewproj *= XMMatrixPerspectiveFovRH(XM_PIDIV2,
		static_cast<float>(wSize.width) / static_cast<float>(wSize.height),
		0.1f,
		300.0f);
	result = transformBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBasicMatrix_));
	assert(SUCCEEDED(result));

	mappedBasicMatrix_->viewproj = viewproj;
	mappedBasicMatrix_->world = world;
	XMVECTOR plane = { 0,1,0,0 };
	XMVECTOR light = { -1,1,-1,0 };
	mappedBasicMatrix_->lightPos = light;
	mappedBasicMatrix_->shadow = XMMatrixShadow(plane, light);

	XMVECTOR lightPos = { -10,30,30,1 };
	XMVECTOR targetPos = { 10,0,0,1 };
	auto screenSize = Application::Instance().GetWindowSize();
	float aspect = static_cast<float>(screenSize.width) / screenSize.height;
	mappedBasicMatrix_->lightViewProj = XMMatrixLookAtRH(lightPos, targetPos, { 0,1,0,0 }) *
		XMMatrixOrthographicRH(30.f, 30.f, 0.1f, 300.f);


	transformBuffer_->Unmap(0, nullptr);

	auto cbDesc = transformBuffer_->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = transformBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbDesc.Width;
	auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
	m_device->CreateConstantBufferView(&cbvDesc, heapPos);

	return true;
}


bool PMDModel::CreateBoneBuffer()
{
	// take bone's name and bone's index to boneTable
	// <bone's name, bone's index>

	boneBuffer_ = Dx12Helper::CreateBuffer(m_device,
		Dx12Helper::AlignedConstantBufferMemory(sizeof(XMMATRIX) * 512));
	auto result = boneBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBoneMatrix_));

	UpdateMotionTransform();

	auto cbDesc = boneBuffer_->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = boneBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbDesc.Width;
	auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
	auto heapIncreSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	heapPos.ptr += heapIncreSize;
	m_device->CreateConstantBufferView(&cbvDesc, heapPos);
	return false;
}

void PMDModel::RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& matrices, size_t index)
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

bool PMDModel::CreateMaterialAndTextureBuffer()
{
	HRESULT result = S_OK;

	auto strideBytes = Dx12Helper::AlignedConstantBufferMemory(sizeof(BasicMaterial));

	materialBuffer_ = Dx12Helper::CreateBuffer(m_device, materials_.size() * strideBytes);

	Dx12Helper::CreateDescriptorHeap(m_device.Get(), materialDescHeap_, materials_.size() * material_descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	auto gpuAddress = materialBuffer_->GetGPUVirtualAddress();
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(materialDescHeap_->GetCPUDescriptorHandleForHeapStart());
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = strideBytes;
	cbvDesc.BufferLocation = gpuAddress;

	// Need to re-watch mapping data
	uint8_t* mappedMaterial = nullptr;
	result = materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterial));
	assert(SUCCEEDED(result));
	for (int i = 0; i < materials_.size(); ++i)
	{
		BasicMaterial* materialData = (BasicMaterial*)mappedMaterial;
		materialData->diffuse = materials_[i].diffuse;
		materialData->alpha = materials_[i].alpha;
		materialData->specular = materials_[i].specular;
		materialData->specularity = materials_[i].specularity;
		materialData->ambient = materials_[i].ambient;
		cbvDesc.BufferLocation = gpuAddress;
		m_device->CreateConstantBufferView(
			&cbvDesc,
			heapAddress);
		// move memory offset
		mappedMaterial += strideBytes;
		gpuAddress += strideBytes;
		heapAddress.Offset(1, heapSize);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ※

		// Create SRV for main texture (image)
		srvDesc.Format = textureBuffers_[i] ? textureBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!textureBuffers_[i].Get() ? whiteTexture_.Get() : textureBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for sphere mapping texture (sph)
		srvDesc.Format = sphBuffers_[i] ? sphBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!sphBuffers_[i].Get() ? whiteTexture_.Get() : sphBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.ptr += heapSize;

		// Create SRV for sphere mapping texture (spa)
		srvDesc.Format = spaBuffers_[i] ? spaBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!spaBuffers_[i].Get() ? blackTexture_.Get() : spaBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for toon map
		srvDesc.Format = toonBuffers_[i] ? toonBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!toonBuffers_[i].Get() ? gradTexture_.Get() : toonBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);
	}
	materialBuffer_->Unmap(0, nullptr);

	return true;
}

bool PMDModel::CreatePipelineStateObject()
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

	psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;     //※ color : ABGR

	// Root Signature
	psoDesc.pRootSignature = rootSig_.Get();

	result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipeline_.GetAddressOf()));
	assert(SUCCEEDED(result));

	return true;
}

void PMDModel::LoadTextureToBuffer()
{
	textureBuffers_.resize(modelPaths_.size());
	sphBuffers_.resize(modelPaths_.size());
	spaBuffers_.resize(modelPaths_.size());
	toonBuffers_.resize(toonPaths_.size());

	for (int i = 0; i < modelPaths_.size(); ++i)
	{
		if (!toonPaths_[i].empty())
		{
			std::string toonPath;
			
			toonPath = StringHelper::GetTexturePathFromModelPath(pmdPath_, toonPaths_[i].c_str());
			toonBuffers_[i] = Dx12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			if (!toonBuffers_[i])
			{
				toonPath = std::string(pmd_path) + "toon/" + toonPaths_[i];
				toonBuffers_[i]= Dx12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			}
		}
		if (!modelPaths_[i].empty())
		{
			auto splittedPaths = StringHelper::SplitFilePath(modelPaths_[i]);
			for (auto& path : splittedPaths)
			{
				auto ext = StringHelper::GetFileExtension(path);
				if (ext == "sph")
				{
					auto sphPath = StringHelper::GetTexturePathFromModelPath(pmdPath_, path.c_str());
					sphBuffers_[i] = Dx12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(sphPath));
				}
				else if (ext == "spa")
				{
					auto spaPath = StringHelper::GetTexturePathFromModelPath(pmdPath_, path.c_str());
					spaBuffers_[i] = Dx12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(spaPath));
				}
				else
				{
					auto texPath = StringHelper::GetTexturePathFromModelPath(pmdPath_, path.c_str());
					textureBuffers_[i] = Dx12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(texPath));
				}
			}
		}
	}

}


bool PMDModel::CreateShadowDepthView(ComPtr<ID3D12Resource>& shadowDepthBuffer_)
{
	Dx12Helper::CreateDescriptorHeap(m_device.Get(), shadowDepthHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(shadowDepthHeap_->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateShaderResourceView(shadowDepthBuffer_.Get(), &srvDesc, heapHandle);

	return false;
}

void PMDModel::CreateRootSignature()
{
	HRESULT result = S_OK;
	//Root Signature
	D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
	rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER rootParam[3] = {};

	// Descriptor table
	D3D12_DESCRIPTOR_RANGE range[4] = {};

	// transform
	range[0] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
		2,                                      // number of descriptors
		0);                                     // base shader register

	// material
	range[1] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	// texture
	range[2] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	range[3] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4);

	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[0], 1, &range[0]);

	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1],
		2,                                          // number of descriptor range
		&range[1],                                  // pointer to descritpor range
		D3D12_SHADER_VISIBILITY_PIXEL);             // shader visibility

	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[2],
		1,                                          // number of descriptor range
		&range[3],                                  // pointer to descritpor range
		D3D12_SHADER_VISIBILITY_PIXEL);             // shader visibility

	rtSigDesc.pParameters = rootParam;
	rtSigDesc.NumParameters = _countof(rootParam);

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
	result = D3D12SerializeRootSignature(&rtSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
		&rootSigBlob,
		&errBlob);
	Dx12Helper::OutputFromErrorBlob(errBlob);
	assert(SUCCEEDED(result));
	result = m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig_.GetAddressOf()));
	assert(SUCCEEDED(result));
}

bool PMDModel::CreateTexture()
{
	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateTransformBuffer();
	CreateBoneBuffer();
	LoadTextureToBuffer();
	CreateMaterialAndTextureBuffer();
	
	return true;
}

void PMDModel::CreateModelPipeline()
{
	CreateRootSignature();
	CreatePipelineStateObject();
}

void PMDModel::UpdateMotionTransform(const size_t& currentFrame)
{
	auto& motionData = vmdMotion_->GetVMDMotionData();
	auto mats = boneMatrices_;

	for (auto& motion : motionData)
	{
		auto index = bonesTable_[motion.first];
		auto& rotationOrigin = m_boneMatrices[index].pos;
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

	RecursiveCalculate(m_boneMatrices, mats, 0);
	std::copy(mats.begin(), mats.end(), mappedBoneMatrix_);
}

float PMDModel::CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n)
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