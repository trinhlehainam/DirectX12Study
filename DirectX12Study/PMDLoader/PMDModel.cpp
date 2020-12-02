#include "PMDModel.h"
#include "../Application.h"
#include "../VMDLoader/VMDMotion.h"
#include "../DirectX12/Dx12Helper.h"
#include <stdio.h>
#include <Windows.h>
#include <array>
#include <d3dcompiler.h>
#include <DirectXTex.h>

namespace
{
	std::string GetTexturePathFromModelPath(const char* modelPath, const char* texturePath)
	{
		std::string ret = modelPath;
		auto idx = ret.rfind("/") + 1;

		return ret.substr(0, idx) + texturePath;
	}
}

namespace
{
	unsigned int AlignedValue(unsigned int value, unsigned int align)
	{
		return value + (align - (value % align)) % align;
	}

	std::wstring ConvertStringToWideString(const std::string& str)
	{
		std::wstring ret;
		auto wstringSize = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), str.length(), nullptr, 0);
		ret.resize(wstringSize);
		MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
			str.c_str(), str.length(), &ret[0], ret.length());
		return ret;
	}

	std::string GetFileExtension(const std::string& path)
	{
		auto idx = path.rfind('.') + 1;
		return idx == std::string::npos ? "" : path.substr(idx, path.length() - idx);
	}

	std::vector<std::string> SplitFilePath(const std::string& path, const char splitter = ' *')
	{
		std::vector <std::string> ret;
		auto idx = path.rfind(splitter);
		if (idx == std::string::npos)
			ret.push_back(path);
		ret.push_back(path.substr(0, idx));
		++idx;
		ret.push_back(path.substr(idx, path.length() - idx));
		return ret;
	}

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
	//// Transform model position
	//// Transform normal vector WHEN rotation
	//for (auto& vertex : vertices_)
	//{
	//	auto& pos = vertex.pos;
	//	auto& normal = vertex.normal;
	//	auto vecPos = XMLoadFloat3(&pos);
	//	auto vecNormal = XMLoadFloat3(&normal);
	//	vecPos = XMVector3Transform(vecPos, transformMatrix);
	//	vecNormal = XMVector4Transform(vecNormal, transformMatrix);
	//	XMStoreFloat3(&pos, vecPos);
	//	XMStoreFloat3(&normal, vecNormal);
	//}
	//std::copy(vertices_.begin(), vertices_.end(), mappedVertex_);

	//// Transform origin rotation position of skinning matrix
	//for (auto& bone : bones_)
	//{
	//	auto& rotationPos = bone.pos;
	//	auto vec = XMLoadFloat3(&rotationPos);
	//	vec = XMVector3Transform(vec, transformMatrix);
	//	XMStoreFloat3(&rotationPos, vec);
	//}

	mappedBasicMatrix_->world *= transformMatrix;
}

uint16_t PMDModel::GetIndices() const
{
	uint16_t ret = 0;
	for (const auto& index : indices_)
	{
		ret += index;
	}
	return ret;
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
	cmdList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)transformDescHeap_.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(0, transformDescHeap_->GetGPUDescriptorHandleForHeapStart());
	/*-------------------------------------------*/

	/*-------------Set up material and texture-------------*/
	cmdList->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)materialDescHeap_.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE materialHeapHandle(materialDescHeap_->GetGPUDescriptorHandleForHeapStart());
	auto materialHeapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32_t indexOffset = 0;
	for (auto& m : materials_)
	{
		cmdList->SetGraphicsRootDescriptorTable(1, materialHeapHandle);

		cmdList->DrawIndexedInstanced(m.indices,
			2,
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

void PMDModel::SetGraphicPinelineState(ComPtr<ID3D12GraphicsCommandList>& cmdList)
{
	cmdList->SetPipelineState(pipeline_.Get());
	cmdList->SetGraphicsRootSignature(rootSig_.Get());
}

BasicMatrix* PMDModel::GetMappedMatrix()
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
	dev_(device)
{
	vmdMotion_ = std::make_shared<VMDMotion>();
}

PMDModel::~PMDModel()
{
	boneBuffer_->Unmap(0, nullptr);
	vertexBuffer_->Unmap(0, nullptr);
	dev_ = nullptr;
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
	bones_.resize(boneNum);
	for (int i = 0; i < boneNum; ++i)
	{
		bones_[i].name = boneData[i].boneName;
		bones_[i].pos = boneData[i].pos;
		bonesTable_[boneData[i].boneName] = i;
	}
	boneMatrices_.resize(boneNum);
	std::fill(boneMatrices_.begin(), boneMatrices_.end(), DirectX::XMMatrixIdentity());

	for (int i = 0; i < boneNum; ++i)
	{
		if (boneData[i].parentNo == 0xffff) continue;
		auto pno = boneData[i].parentNo;
		bones_[pno].children.push_back(i);
#ifdef _DEBUG
		bones_[pno].childrenName.push_back(boneData[i].boneName);
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

	vertexBuffer_ = Dx12Helper::CreateBuffer(dev_, sizeof(vertices_[0]) * vertices_.size());

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

	indicesBuffer_ = Dx12Helper::CreateBuffer(dev_, sizeof(indices_[0]) * indices_.size());

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

	Dx12Helper::CreateDescriptorHeap(dev_, transformDescHeap_, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	transformBuffer_ = Dx12Helper::CreateBuffer(dev_, AlignedValue(sizeof(BasicMatrix), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

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
		XMMatrixOrthographicRH(100.f, 100.f, 0.1f, 300.f);

	transformBuffer_->Unmap(0, nullptr);

	auto cbDesc = transformBuffer_->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = transformBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbDesc.Width;
	auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
	dev_->CreateConstantBufferView(&cbvDesc, heapPos);

	return true;
}


bool PMDModel::CreateBoneBuffer()
{
	// take bone's name and bone's index to boneTable
	// <bone's name, bone's index>

	boneBuffer_ = Dx12Helper::CreateBuffer(dev_, AlignedValue(sizeof(XMMATRIX) * 512, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));
	auto result = boneBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBoneMatrix_));

	UpdateMotionTransform();

	auto cbDesc = boneBuffer_->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = boneBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbDesc.Width;
	auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
	auto heapIncreSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	heapPos.ptr += heapIncreSize;
	dev_->CreateConstantBufferView(&cbvDesc, heapPos);
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

	auto strideBytes = AlignedValue(sizeof(BasicMaterial), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

	materialBuffer_ = Dx12Helper::CreateBuffer(dev_, materials_.size() * strideBytes);

	Dx12Helper::CreateDescriptorHeap(dev_, materialDescHeap_, materials_.size() * material_descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	auto gpuAddress = materialBuffer_->GetGPUVirtualAddress();
	auto heapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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
		dev_->CreateConstantBufferView(
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
		dev_->CreateShaderResourceView(
			!textureBuffers_[i].Get() ? whiteTexture_.Get() : textureBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for sphere mapping texture (sph)
		srvDesc.Format = sphBuffers_[i] ? sphBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		dev_->CreateShaderResourceView(
			!sphBuffers_[i].Get() ? whiteTexture_.Get() : sphBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.ptr += heapSize;

		// Create SRV for sphere mapping texture (spa)
		srvDesc.Format = spaBuffers_[i] ? spaBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		dev_->CreateShaderResourceView(
			!spaBuffers_[i].Get() ? blackTexture_.Get() : spaBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for toon map
		srvDesc.Format = toonBuffers_[i] ? toonBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		dev_->CreateShaderResourceView(
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
	ComPtr<ID3DBlob> errBlob;
	ComPtr<ID3DBlob> vsBlob;
	result = D3DCompileFromFile(
		L"shader/vs.hlsl",                                  // path to shader file
		nullptr,                                            // define marcro object 
		D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
		"VS",                                               // entry name
		"vs_5_1",                                           // shader version
		0, 0,                                               // Flag1, Flag2 (unknown)
		vsBlob.GetAddressOf(),
		errBlob.GetAddressOf());
	Dx12Helper::OutputFromErrorBlob(errBlob);
	assert(SUCCEEDED(result));
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

	// Rasterizer
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.RasterizerState.FrontCounterClockwise = true;
	psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// Pixel Shader
	ComPtr<ID3DBlob> psBlob;
	result = D3DCompileFromFile(
		L"shader/ps.hlsl",                              // path to shader file
		nullptr,                                        // define marcro object 
		D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
		"PS",                                           // entry name
		"ps_5_1",                                       // shader version
		0, 0,                                           // Flag1, Flag2 (unknown)
		psBlob.GetAddressOf(),
		errBlob.GetAddressOf());
	Dx12Helper::OutputFromErrorBlob(errBlob);
	assert(SUCCEEDED(result));
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

	result = dev_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(pipeline_.GetAddressOf()));
	assert(SUCCEEDED(result));

	return true;
}

bool PMDModel::CreateTextureFromFilePath(const std::wstring& path, ComPtr<ID3D12Resource>& buffer)
{
	HRESULT result = S_OK;

	TexMetadata metadata;
	ScratchImage scratch;
	result = DirectX::LoadFromWICFile(
		path.c_str(),
		WIC_FLAGS_IGNORE_SRGB,
		&metadata,
		scratch);
	//return SUCCEEDED(result);
	if (FAILED(result)) return false;

	// Create buffer
	D3D12_HEAP_PROPERTIES heapProp = {};
	heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

	D3D12_RESOURCE_DESC rsDesc = {};
	rsDesc.Format = metadata.format;
	rsDesc.Width = metadata.width;
	rsDesc.Height = metadata.height;
	rsDesc.DepthOrArraySize = metadata.arraySize;
	rsDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
	rsDesc.MipLevels = metadata.mipLevels;
	rsDesc.SampleDesc.Count = 1;
	rsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	result = dev_->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&rsDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer.GetAddressOf())
	);
	assert(SUCCEEDED(result));

	auto image = scratch.GetImage(0, 0, 0);
	result = buffer->WriteToSubresource(0,
		nullptr,
		image->pixels,
		image->rowPitch,
		image->slicePitch);
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

			toonPath = GetTexturePathFromModelPath(pmdPath_, toonPaths_[i].c_str());
			if (!CreateTextureFromFilePath(ConvertStringToWideString(toonPath), toonBuffers_[i]))
			{
				toonPath = std::string(pmd_path) + "toon/" + toonPaths_[i];
				CreateTextureFromFilePath(ConvertStringToWideString(toonPath), toonBuffers_[i]);
			}
		}
		if (!modelPaths_[i].empty())
		{
			auto splittedPaths = SplitFilePath(modelPaths_[i]);
			for (auto& path : splittedPaths)
			{
				auto ext = GetFileExtension(path);
				if (ext == "sph")
				{
					auto sphPath = GetTexturePathFromModelPath(pmdPath_, path.c_str());
					CreateTextureFromFilePath(ConvertStringToWideString(sphPath), sphBuffers_[i]);
				}
				else if (ext == "spa")
				{
					auto spaPath = GetTexturePathFromModelPath(pmdPath_, path.c_str());
					CreateTextureFromFilePath(ConvertStringToWideString(spaPath), spaBuffers_[i]);
				}
				else
				{
					auto texPath = GetTexturePathFromModelPath(pmdPath_, path.c_str());
					CreateTextureFromFilePath(ConvertStringToWideString(texPath), textureBuffers_[i]);
				}
			}
		}
	}

}

void PMDModel::CreateRootSignature()
{
	HRESULT result = S_OK;
	//Root Signature
	D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
	rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	D3D12_ROOT_PARAMETER rootParam[2] = {};

	// Descriptor table
	D3D12_DESCRIPTOR_RANGE range[3] = {};

	// transform
	range[0] = CD3DX12_DESCRIPTOR_RANGE(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
		2,                                      // number of descriptors
		0);                                     // base shader register

	// material
	range[1] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);

	// texture
	range[2] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[0], 1, &range[0]);

	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1],
		2,                                          // number of descriptor range
		&range[1],                                  // pointer to descritpor range
		D3D12_SHADER_VISIBILITY_PIXEL);             // shader visibility

	rtSigDesc.pParameters = rootParam;
	rtSigDesc.NumParameters = _countof(rootParam);

	//サンプラらの定義、サンプラとはUVが0未満とか1超えとかのときの
	//動きyや、UVをもとに色をとってくるときのルールを指定するもの
	D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
	//WRAPは繰り返しを表す。
	CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0],
		0,                                  // shader register location
		D3D12_FILTER_MIN_MAG_MIP_POINT);    // Filter     

	samplerDesc[1] = samplerDesc[0];
	samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	samplerDesc[1].ShaderRegister = 1;

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
	result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig_.GetAddressOf()));
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
		auto& rotationOrigin = bones_[index].pos;
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

	RecursiveCalculate(bones_, mats, 0);
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