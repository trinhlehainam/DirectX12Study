#include "PMDModel.h"
#include "../Application.h"
#include "../VMDLoader/VMDMotion.h"
#include "../Utility/D12Helper.h"
#include "../Utility/StringHelper.h"
#include "PMDLoader.h"

#include <stdio.h>
#include <Windows.h>
#include <array>
#include <d3dcompiler.h>

namespace
{
	const char* pmd_path = "resource/PMD/";
	constexpr unsigned int material_descriptor_count = 5;
}

PMDModel::PMDModel()
{
	m_vmdMotion = std::make_shared<VMDMotion>();
	m_pmdLoader = std::make_unique<PMDLoader>();
}

PMDModel::PMDModel(ComPtr<ID3D12Device> device) :
	m_device(device)
{
	m_vmdMotion = std::make_shared<VMDMotion>();
	m_pmdLoader = std::make_unique<PMDLoader>();
}

PMDModel::~PMDModel()
{
	m_device = nullptr;
	m_whiteTexture = nullptr;
	m_blackTexture = nullptr;
	m_gradTexture = nullptr;
}

void PMDModel::CreateModel(ID3D12GraphicsCommandList* cmdList)
{
	CreateTransformConstant();
	LoadTextureToBuffer();
	CreateMaterialAndTextureBuffer(cmdList);
}

void PMDModel::Transform(const DirectX::XMMATRIX& transformMatrix)
{
	m_transformBuffer.MappedData().world *= transformMatrix;
}

void PMDModel::LoadMotion(const char* path)
{
	m_vmdMotion->Load(path);
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
	cmdList->SetDescriptorHeaps(1, m_materialDescHeap.GetAddressOf());
	CD3DX12_GPU_DESCRIPTOR_HANDLE materialHeapHandle(m_materialDescHeap->GetGPUDescriptorHandleForHeapStart());
	auto materialHeapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	uint32_t indexOffset = StartIndexLocation;
	for (auto& m : m_subMaterials)
	{
		cmdList->SetGraphicsRootDescriptorTable(3, materialHeapHandle);

		cmdList->DrawIndexedInstanced(m.indexCount,
			1,
			indexOffset,
			BaseVertexLocation,
			0);
		indexOffset += m.indexCount;
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

	cmdList->DrawIndexedInstanced(m_pmdLoader->m_indices.size(), 1, StartIndexLocation, BaseVertexLocation, 0);
}

const std::vector<uint16_t>& PMDModel::Indices() const
{
	return m_pmdLoader->m_indices;
}

const std::vector<PMDVertex>& PMDModel::Vertices() const
{
	return m_pmdLoader->m_vertices;
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

bool PMDModel::LoadPMD(const char* path)
{
	m_pmdLoader->Load(path);
	m_subMaterials = std::move(m_pmdLoader->m_subMaterials);
	return true;
}

bool PMDModel::CreateBoneBuffer()
{
	// take bone's name and bone's index to boneTable
	// <bone's name, bone's index>

	boneBuffer_ = D12Helper::CreateBuffer(m_device.Get(),
		D12Helper::AlignedConstantBufferMemory(sizeof(XMMATRIX) * 512));
	auto result = boneBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedBoneMatrix));

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
	auto bones = m_pmdLoader->m_boneMatrices;
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

bool PMDModel::CreateMaterialAndTextureBuffer(ID3D12GraphicsCommandList* cmdList)
{
	HRESULT result = S_OK;

	size_t strideBytes = D12Helper::AlignedConstantBufferMemory(sizeof(PMDMaterial));
	size_t sizeInBytes = m_pmdLoader->m_materials.size() * strideBytes;

	m_materialBuffer = D12Helper::CreateBuffer(m_device.Get(), sizeInBytes);

	D12Helper::CreateDescriptorHeap(m_device.Get(), m_materialDescHeap, m_pmdLoader->m_materials.size() * material_descriptor_count, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

	auto gpuAddress = m_materialBuffer->GetGPUVirtualAddress();
	auto heapSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(m_materialDescHeap->GetCPUDescriptorHandleForHeapStart());
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = strideBytes;
	cbvDesc.BufferLocation = gpuAddress;

	// Map materials data to materials default buffer
	uint8_t* m_mappedMaterial = nullptr;
	result = m_materialBuffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedMaterial));
	assert(SUCCEEDED(result));
	for (int i = 0; i < m_pmdLoader->m_materials.size(); ++i)
	{
		PMDMaterial* mappedData = (PMDMaterial*)m_mappedMaterial;
		mappedData->diffuse = m_pmdLoader->m_materials[i].diffuse;
		mappedData->alpha = m_pmdLoader->m_materials[i].alpha;
		mappedData->specular = m_pmdLoader->m_materials[i].specular;
		mappedData->specularity = m_pmdLoader->m_materials[i].specularity;
		mappedData->ambient = m_pmdLoader->m_materials[i].ambient;
		cbvDesc.BufferLocation = gpuAddress;
		m_device->CreateConstantBufferView(
			&cbvDesc,
			heapAddress);
		// move memory offset
		m_mappedMaterial += strideBytes;
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
		srvDesc.Format = m_textureBuffer[i] ? m_textureBuffer[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!m_textureBuffer[i].Get() ? m_whiteTexture : m_textureBuffer[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for sphere mapping texture (sph)
		srvDesc.Format = m_sphBuffers[i] ? m_sphBuffers[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!m_sphBuffers[i].Get() ? m_whiteTexture : m_sphBuffers[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.ptr += heapSize;

		// Create SRV for sphere mapping texture (spa)
		srvDesc.Format = m_spaBuffers[i] ? m_spaBuffers[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!m_spaBuffers[i].Get() ? m_blackTexture : m_spaBuffers[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);

		// Create SRV for toon map
		srvDesc.Format = m_toonBuffers[i] ? m_toonBuffers[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
		m_device->CreateShaderResourceView(
			!m_toonBuffers[i].Get() ? m_gradTexture : m_toonBuffers[i].Get(),
			&srvDesc,
			heapAddress
		);
		heapAddress.Offset(1, heapSize);
	}
	m_materialBuffer->Unmap(0, nullptr);

	return true;
}

void PMDModel::LoadTextureToBuffer()
{
	m_textureBuffer.resize(m_pmdLoader->m_modelPaths.size());
	m_sphBuffers.resize(m_pmdLoader->m_modelPaths.size());
	m_spaBuffers.resize(m_pmdLoader->m_modelPaths.size());
	m_toonBuffers.resize(m_pmdLoader->m_toonPaths.size());

	for (int i = 0; i < m_textureBuffer.size(); ++i)
	{
		if (!m_pmdLoader->m_toonPaths[i].empty())
		{
			std::string toonPath;
			
			toonPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->m_path, m_pmdLoader->m_toonPaths[i].c_str());
			m_toonBuffers[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			if (!m_toonBuffers[i])
			{
				toonPath = std::string(pmd_path) + "toon/" + m_pmdLoader->m_toonPaths[i];
				m_toonBuffers[i]= D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(toonPath));
			}
		}
		if (!m_pmdLoader->m_toonPaths[i].empty())
		{
			auto splittedPaths = StringHelper::SplitFilePath(m_pmdLoader->m_modelPaths[i]);
			for (auto& path : splittedPaths)
			{
				auto ext = StringHelper::GetFileExtension(path);
				if (ext == "sph")
				{
					auto sphPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->m_path, path.c_str());
					m_sphBuffers[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(sphPath));
				}
				else if (ext == "spa")
				{
					auto spaPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->m_path, path.c_str());
					m_spaBuffers[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(spaPath));
				}
				else
				{
					auto texPath = StringHelper::GetTexturePathFromModelPath(m_pmdLoader->m_path, path.c_str());
					m_textureBuffer[i] = D12Helper::CreateTextureFromFilePath(m_device, StringHelper::ConvertStringToWideString(texPath));
				}
			}
		}
	}

}

void PMDModel::UpdateMotionTransform(const size_t& currentFrame)
{
	/*auto& motionData = vmdMotion_->GetVMDMotionData();
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
	std::copy(mats.begin(), mats.end(), mappedBoneMatrix_);*/
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