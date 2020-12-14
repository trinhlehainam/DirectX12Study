#include "PMDModel.h"
#include "../Application.h"
#include "../VMDLoader/VMDMotion.h"
#include "../Utility/D12Helper.h"
#include "../Utility/StringHelper.h"

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
	CreateTransformConstant();
	LoadTextureToBuffer();
	CreateMaterialAndTextureBuffer();
}

void PMDModel::Transform(const DirectX::XMMATRIX& transformMatrix)
{
	m_transformBuffer.MappedData().world *= transformMatrix;
}

void PMDModel::LoadMotion(const char* path)
{
	vmdMotion_->Load(path);
}

void PMDModel::Render(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
	const uint32_t& BaseVertexLocation)
{
	//auto frame = frameNO % vmdMotion_->GetMaxFrame();
	//UpdateMotionTransform(frame);

	/*-------------Set up transform-------------*/
	cmdList->SetDescriptorHeaps(1, m_transformDescHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(2, m_transformDescHeap->GetGPUDescriptorHandleForHeapStart());
	/*-------------------------------------------*/

	/*-------------Set up material and texture-------------*/
	cmdList->SetDescriptorHeaps(1, materialDescHeap_.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE materialHeapHandle(materialDescHeap_->GetGPUDescriptorHandleForHeapStart());
	auto materialHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32_t indexOffset = StartIndexLocation;
	for (auto& m : materials_)
	{
		cmdList->SetGraphicsRootDescriptorTable(3, materialHeapHandle);

		cmdList->DrawIndexedInstanced(m.indices,
			1,
			indexOffset,
			BaseVertexLocation,
			0);
		indexOffset += m.indices;
		materialHeapHandle.Offset(material_descriptor_count, materialHeapSize);
	}
	/*-------------------------------------------*/
}

void PMDModel::RenderDepth(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
	const uint32_t& BaseVertexLocation)
{
	// Object constant
	cmdList->SetDescriptorHeaps(1, m_transformDescHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(1, m_transformDescHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->DrawIndexedInstanced(indices_.size(), 1, StartIndexLocation, BaseVertexLocation, 0);
}

void PMDModel::GetDefaultTexture(ID3D12Resource* whiteTexture, 
	ID3D12Resource* blackTexture, ID3D12Resource* gradTexture)
{
	m_whiteTexture = whiteTexture;
	m_blackTexture = blackTexture;
	m_gradTexture = gradTexture;
}

PMDModel::PMDModel(ComPtr<ID3D12Device> device) :
	m_device(device)
{
	vmdMotion_ = std::make_shared<VMDMotion>();
}

PMDModel::~PMDModel()
{
	m_device = nullptr;
	m_whiteTexture = nullptr;
	m_blackTexture = nullptr;
	m_gradTexture = nullptr;
}

bool PMDModel::LoadPMD(const char* path)
{
	pmdPath_ = path;

	//Ž¯•ÊŽq"pmd"
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
	m_bones.resize(boneNum);
	for (int i = 0; i < boneNum; ++i)
	{
		m_bones[i].name = boneData[i].boneName;
		m_bones[i].pos = boneData[i].pos;
		bonesTable_[boneData[i].boneName] = i;
	}
	boneMatrices_.resize(512);
	std::fill(boneMatrices_.begin(), boneMatrices_.end(), DirectX::XMMatrixIdentity());

	for (int i = 0; i < boneNum; ++i)
	{
		if (boneData[i].parentNo == 0xffff) continue;
		auto pno = boneData[i].parentNo;
		m_bones[pno].children.push_back(i);
#ifdef _DEBUG
		m_bones[pno].childrenName.push_back(boneData[i].boneName);
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

bool PMDModel::CreateBoneBuffer()
{
	// take bone's name and bone's index to boneTable
	// <bone's name, bone's index>

	boneBuffer_ = D12Helper::CreateBuffer(m_device.Get(),
		D12Helper::AlignedConstantBufferMemory(sizeof(XMMATRIX) * 512));
	auto result = boneBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBoneMatrix_));

	UpdateMotionTransform();

	auto cbDesc = boneBuffer_->GetDesc();
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = boneBuffer_->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = cbDesc.Width;
	//auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
	//auto heapIncreSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//heapPos.ptr += heapIncreSize;
	//m_device->CreateConstantBufferView(&cbvDesc, heapPos);
	return false;
}

bool PMDModel::CreateTransformConstant()
{
	D12Helper::CreateDescriptorHeap(m_device.Get(), m_transformDescHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_transformBuffer.Create(m_device.Get(), 1, true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_transformBuffer.GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = m_transformBuffer.SizeInBytes();

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_transformDescHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

	auto& data = m_transformBuffer.MappedData();
	data.world = XMMatrixIdentity();
	auto bones = boneMatrices_;
	std::copy(bones.begin(), bones.end(), data.bones);

	return true;
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

	auto strideBytes = D12Helper::AlignedConstantBufferMemory(sizeof(BasicMaterial));

	materialBuffer_ = D12Helper::CreateBuffer(m_device.Get(), materials_.size() * strideBytes);

	D12Helper::CreateDescriptorHeap(m_device.Get(), materialDescHeap_, materials_.size() * material_descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

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
		srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ¦

		// Create SRV for main texture (image)
		srvDesc.Format = textureBuffers_[i] ? textureBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!textureBuffers_[i].Get() ? m_whiteTexture : textureBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for sphere mapping texture (sph)
		srvDesc.Format = sphBuffers_[i] ? sphBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!sphBuffers_[i].Get() ? m_whiteTexture : sphBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.ptr += heapSize;

		// Create SRV for sphere mapping texture (spa)
		srvDesc.Format = spaBuffers_[i] ? spaBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!spaBuffers_[i].Get() ? m_blackTexture : spaBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for toon map
		srvDesc.Format = toonBuffers_[i] ? toonBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!toonBuffers_[i].Get() ? m_gradTexture : toonBuffers_[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);
	}
	materialBuffer_->Unmap(0, nullptr);

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
			toonBuffers_[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			if (!toonBuffers_[i])
			{
				toonPath = std::string(pmd_path) + "toon/" + toonPaths_[i];
				toonBuffers_[i]= D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
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
					sphBuffers_[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(sphPath));
				}
				else if (ext == "spa")
				{
					auto spaPath = StringHelper::GetTexturePathFromModelPath(pmdPath_, path.c_str());
					spaBuffers_[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(spaPath));
				}
				else
				{
					auto texPath = StringHelper::GetTexturePathFromModelPath(pmdPath_, path.c_str());
					textureBuffers_[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(texPath));
				}
			}
		}
	}

}

void PMDModel::UpdateMotionTransform(const size_t& currentFrame)
{
	auto& motionData = vmdMotion_->GetVMDMotionData();
	auto mats = boneMatrices_;

	for (auto& motion : motionData)
	{
		auto index = bonesTable_[motion.first];
		auto& rotationOrigin = m_bones[index].pos;
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

	RecursiveCalculate(m_bones, mats, 0);
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