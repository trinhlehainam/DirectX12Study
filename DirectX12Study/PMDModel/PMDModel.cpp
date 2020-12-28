#include "PMDModel.h"
#include "../Application.h"
#include "VMD/VMDMotion.h"
#include "../Utility/D12Helper.h"
#include "../Utility/StringHelper.h"
#include "PMDLoader.h"
#include "../common.h"

#include <stdio.h>
#include <Windows.h>
#include <array>
#include <d3dcompiler.h>

namespace
{
	const char* pmd_path = "resource/PMD/";
	constexpr unsigned int material_descriptor_count_per_heap = 5;
}

PMDModel::PMDModel()
{
	m_pmdLoader = std::make_unique<PMDLoader>();
}

PMDModel::PMDModel(ID3D12Device* device) :
	m_device(device)
{
	m_pmdLoader = std::make_unique<PMDLoader>();
}

PMDModel::~PMDModel()
{
	m_device = nullptr;
	m_whiteTexture = nullptr;
	m_blackTexture = nullptr;
	m_gradTexture = nullptr;
	m_vmdMotion = nullptr;
}

void PMDModel::CreateModel(ID3D12GraphicsCommandList* cmdList)
{
	CreateTransformConstantBuffer();
	LoadTextureToBuffer();
	CreateMaterialAndTextureBuffer(cmdList);
}

void PMDModel::Transform(const DirectX::XMMATRIX& transformMatrix)
{
	Resource.TransformConstant.HandleMappedData()->world *= transformMatrix;
}

void PMDModel::Play(VMDMotion* animation)
{
	m_vmdMotion = animation;
	// Init bones transform;
	UpdateMotionTransform();
}

void PMDModel::Move(const float& moveX, const float& moveY, const float& moveZ)
{
	Transform(XMMatrixTranslation(moveX, moveY, moveZ));
}

void PMDModel::RotateX(const float& angle)
{
	Transform(XMMatrixRotationX(angle));
}

void PMDModel::RotateY(const float& angle)
{
	Transform(XMMatrixRotationY(angle));
}

void PMDModel::RotateZ(const float& angle)
{
	Transform(XMMatrixRotationZ(angle));
}

void PMDModel::Scale(const float& scaleX, const float& scaleY, const float& scaleZ)
{
	Transform(XMMatrixScaling(scaleX, scaleY, scaleZ));
}

void PMDModel::Render(ID3D12GraphicsCommandList* cmdList, const uint32_t& StartIndexLocation, 
	const uint32_t& BaseVertexLocation)
{
	/*-------------Set up transform-------------*/
	cmdList->SetDescriptorHeaps(1, RenderResource.TransformHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(2, RenderResource.TransformHeap->GetGPUDescriptorHandleForHeapStart());
	/*-------------------------------------------*/

	/*-------------Set up material and texture-------------*/
	cmdList->SetDescriptorHeaps(1, RenderResource.MaterialHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE materialHeapHandle(RenderResource.MaterialHeap->GetGPUDescriptorHandleForHeapStart());
	auto materialHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32_t indexOffset = StartIndexLocation;
	for (auto& m : RenderResource.SubMaterials)
	{
		cmdList->SetGraphicsRootDescriptorTable(3, materialHeapHandle);

		cmdList->DrawIndexedInstanced(m.indexCount,
			1,
			indexOffset,
			BaseVertexLocation,
			0);
		indexOffset += m.indexCount;
		materialHeapHandle.Offset(material_descriptor_count_per_heap, materialHeapSize);
	}
	/*-------------------------------------------*/
}

void PMDModel::RenderDepth(ID3D12GraphicsCommandList* cmdList, const uint32_t& IndexCount, const uint32_t& StartIndexLocation,
	const uint32_t& BaseVertexLocation)
{
	// Object constant
	cmdList->SetDescriptorHeaps(1, RenderResource.TransformHeap.GetAddressOf());
	cmdList->SetGraphicsRootDescriptorTable(1, RenderResource.TransformHeap->GetGPUDescriptorHandleForHeapStart());

	cmdList->DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void PMDModel::Update(const float& deltaTime)
{
	constexpr float animation_speed = 50.0f/second_to_millisecond;
	if (m_timer <= 0.0f)
	{
		m_timer += animation_speed;
		++m_frameCnt;
		UpdateMotionTransform(m_frameCnt);
	}
	m_timer -= deltaTime;
}

const std::vector<uint16_t>& PMDModel::Indices() const
{
	return m_pmdLoader->Indices;
}

const std::vector<PMDVertex>& PMDModel::Vertices() const
{
	return m_pmdLoader->Vertices;
}

void PMDModel::ClearSubresources()
{
	m_pmdLoader.reset();
}

void PMDModel::SetDefaultTexture(ID3D12Resource* whiteTexture, 
	ID3D12Resource* blackTexture, ID3D12Resource* gradTexture)
{
	m_whiteTexture = whiteTexture;
	m_blackTexture = blackTexture;
	m_gradTexture = gradTexture;
}

void PMDModel::SetDevice(ID3D12Device* pDevice)
{
	m_device = pDevice;
}

bool PMDModel::Load(const char* path)
{
	m_pmdLoader->Load(path);
	RenderResource.SubMaterials = std::move(m_pmdLoader->SubMaterials);
	// Info for animation
	m_bones = std::move(m_pmdLoader->Bones);
	m_bonesTable = std::move(m_pmdLoader->BoneTable);
	m_boneMatrices.resize(512);
	std::fill(m_boneMatrices.begin(), m_boneMatrices.end(), DirectX::XMMatrixIdentity());
	return true;
}

bool PMDModel::CreateTransformConstantBuffer()
{
	D12Helper::CreateDescriptorHeap(m_device, RenderResource.TransformHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	Resource.TransformConstant.Create(m_device, 1, true);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = Resource.TransformConstant.GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = Resource.TransformConstant.SizeInBytes();

	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(RenderResource.TransformHeap->GetCPUDescriptorHandleForHeapStart());
	m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

	auto mappedData = Resource.TransformConstant.HandleMappedData();
	mappedData->world = XMMatrixIdentity();
	auto bones = m_boneMatrices;

	std::copy(bones.begin(), bones.end(), mappedData->bones);

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

bool PMDModel::CreateMaterialAndTextureBuffer(ID3D12GraphicsCommandList* cmdList)
{
	HRESULT result = S_OK;
	const size_t num_materials_block = m_pmdLoader->Materials.size();

	D12Helper::CreateDescriptorHeap(m_device, RenderResource.MaterialHeap, num_materials_block * material_descriptor_count_per_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart());
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//
	// Map materials data to materials default buffer
	//
	size_t strideBytes = D12Helper::AlignedConstantBufferMemory(sizeof(PMDMaterial));
	size_t sizeInBytes = num_materials_block * strideBytes;

	Resource.MaterialConstant = D12Helper::CreateBuffer(m_device, sizeInBytes);
	auto gpuAddress = Resource.MaterialConstant->GetGPUVirtualAddress();

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = strideBytes;
	cbvDesc.BufferLocation = gpuAddress;

	// Material constant
	uint8_t* m_mappedMaterial = nullptr;
	result = Resource.MaterialConstant->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedMaterial));
	assert(SUCCEEDED(result));
	auto& material = m_pmdLoader->Materials;
	// index of material on each block
	size_t index = 0;
	for (int i = 0; i < num_materials_block; ++i)
	{
		auto mappedData = reinterpret_cast<PMDMaterial*>(m_mappedMaterial);
		(*mappedData) = material[i];
		cbvDesc.BufferLocation = gpuAddress;
		m_device->CreateConstantBufferView(
			&cbvDesc,
			heapAddress);
		// move memory offset
		m_mappedMaterial += strideBytes;
		gpuAddress += strideBytes;
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	Resource.MaterialConstant->Unmap(0, nullptr);
	++index;

	// Material texture
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.PlaneSlice = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
	srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ¦

	// Texture
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for main texture (image)
		srvDesc.Format = Resource.Textures[i] ? Resource.Textures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.Textures[i].Get() ? m_whiteTexture : Resource.Textures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	++index;
	// sph
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for sphere mapping texture (sph)
		srvDesc.Format = Resource.sphTextures[i] ? Resource.sphTextures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.sphTextures[i].Get() ? m_whiteTexture : Resource.sphTextures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	++index;
	// spa
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for sphere mapping texture (spa)
		srvDesc.Format = Resource.spaTextures[i] ? Resource.spaTextures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.spaTextures[i].Get() ? m_blackTexture : Resource.spaTextures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}
	++index;
	// toon
	heapAddress = RenderResource.MaterialHeap->GetCPUDescriptorHandleForHeapStart();
	heapAddress.Offset(index, heapSize);
	for (int i = 0; i < num_materials_block; ++i)
	{
		// Create SRV for toon map
		srvDesc.Format = Resource.ToonTextures[i] ? Resource.ToonTextures[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!Resource.ToonTextures[i].Get() ? m_gradTexture : Resource.ToonTextures[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(material_descriptor_count_per_heap, heapSize);
	}

	return true;
}

void PMDModel::LoadTextureToBuffer()
{
	Resource.Textures.resize(m_pmdLoader->ModelPaths.size());
	Resource.sphTextures.resize(m_pmdLoader->ModelPaths.size());
	Resource.spaTextures.resize(m_pmdLoader->ModelPaths.size());
	Resource.ToonTextures.resize(m_pmdLoader->ToonPaths.size());

	auto& Textures = Resource.Textures;
	auto& sphTextures = Resource.sphTextures;
	auto& spaTextures = Resource.spaTextures;
	auto& ToonTextures = Resource.ToonTextures;

	for (int i = 0; i < Textures.size(); ++i)
	{
		if (!m_pmdLoader->ToonPaths[i].empty())
		{
			std::string toonPath;
			
			toonPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, m_pmdLoader->ToonPaths[i].c_str());
			ToonTextures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			if (!ToonTextures[i])
			{
				toonPath = std::string(pmd_path) + "toon/" + m_pmdLoader->ToonPaths[i];
				ToonTextures[i]= D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			}
		}
		if (!m_pmdLoader->ToonPaths[i].empty())
		{
			auto splittedPaths = StringHelper::SplitFilePath(m_pmdLoader->ModelPaths[i]);
			for (auto& path : splittedPaths)
			{
				auto ext = StringHelper::GetFileExtension(path);
				if (ext == "sph")
				{
					auto sphPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, path.c_str());
					sphTextures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(sphPath));
				}
				else if (ext == "spa")
				{
					auto spaPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, path.c_str());
					spaTextures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(spaPath));
				}
				else
				{
					auto texPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->Path, path.c_str());
					Textures[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(texPath));
				}
			}
		}
	}

}

void PMDModel::UpdateMotionTransform(const size_t& currentFrame)
{
	// If model don't have animtion, don't need to do motion
	if (!m_vmdMotion) return;

	auto& motionData = m_vmdMotion->GetVMDMotionData();
	auto mats = m_boneMatrices;

	for (auto& motion : motionData)
	{
		auto index = m_bonesTable[motion.first];
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
	auto mappedBones = Resource.TransformConstant.HandleMappedData();
	std::copy(mats.begin(), mats.end(), mappedBones->bones);
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