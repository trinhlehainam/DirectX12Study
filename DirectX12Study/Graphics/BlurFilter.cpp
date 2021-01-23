#include "BlurFilter.h"

#include <vector>
#include <cassert>

#include "RootSignature.h"
#include "../Utility/D12Helper.h"

#define IMPL (*m_impl)
using Microsoft::WRL::ComPtr;

namespace
{
	constexpr UINT max_blur_radius = 5;
}

class BlurFilter::Impl
{
	friend BlurFilter;
private:
	bool CreateWeights();
	bool CreateRootSignature(ID3D12Device* pDevice);
	bool CreatePSO(ID3D12Device* pDevice);
	bool CreateResources(ID3D12Device* pDevice, UINT texWidth, UINT texHeight, DXGI_FORMAT texFormat);
	bool CreateDescriptors(ID3D12Device* pDevice);
	std::vector<float> CalculateGaussianWeights(float sigma);
private:
	ComPtr<ID3D12RootSignature> m_rootSig;
	ComPtr<ID3D12PipelineState> m_pso;
	ComPtr<ID3D12Resource> m_mainTex;
	ComPtr<ID3D12Resource> m_subTex;
	ComPtr<ID3D12DescriptorHeap> m_heap;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_heapHandle;
	UINT m_heapSize;

	std::vector<float> m_weights;
	int m_blurRadius = 0;
};

bool BlurFilter::Impl::CreateWeights()
{
	m_weights = CalculateGaussianWeights(2.5f);
	m_blurRadius = m_weights.size() / 2;
	return true;
}

bool BlurFilter::Impl::CreateRootSignature(ID3D12Device* pDevice)
{
	RootSignature rootSig;
	rootSig.AddRootParameterAs32BitsConstants(12);
	rootSig.AddRootParameterAsDescriptorTable(0, 1, 1);
	rootSig.Create(pDevice);

	m_rootSig = rootSig.Get();
	return true;
}

bool BlurFilter::Impl::CreatePSO(ID3D12Device* pDevice)
{
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	ComPtr<ID3DBlob> csBlob = D12Helper::CompileShaderFromFile(L"shader/BlurFilter.hlsl", "BlurFilter", "cs_5_1");
	psoDesc.CS = CD3DX12_SHADER_BYTECODE(csBlob.Get());
	psoDesc.pRootSignature = m_rootSig.Get();
	
	pDevice->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(m_pso.GetAddressOf()));

	return true;
}

bool BlurFilter::Impl::CreateResources(ID3D12Device* pDevice, UINT texWidth, UINT texHeight, DXGI_FORMAT texFormat)
{
	m_mainTex = D12Helper::CreateTexture2D(pDevice, texWidth, texHeight, texFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, 
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_COMMON);

	m_subTex = D12Helper::CreateTexture2D(pDevice, texWidth, texHeight, texFormat,
		D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS,
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_RESOURCE_STATE_COMMON);

	return true;
}

bool BlurFilter::Impl::CreateDescriptors(ID3D12Device* pDevice)
{
	D12Helper::CreateDescriptorHeap(pDevice, m_heap, 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
	m_heap->SetName(L"Blur-Heap");
	CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_heap->GetCPUDescriptorHandleForHeapStart());
	const auto heap_size = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_heapSize = heap_size;

	auto texDesc = m_mainTex->GetDesc();
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = texDesc.Format;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = texDesc.Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;

	pDevice->CreateShaderResourceView(m_mainTex.Get(), &srvDesc, heapHandle);
	heapHandle.Offset(1, heap_size);
	pDevice->CreateUnorderedAccessView(m_subTex.Get(), nullptr, &uavDesc, heapHandle);
	heapHandle.Offset(1, heap_size);

	pDevice->CreateShaderResourceView(m_subTex.Get(), &srvDesc, heapHandle);
	heapHandle.Offset(1, heap_size);
	pDevice->CreateUnorderedAccessView(m_mainTex.Get(), nullptr, &uavDesc, heapHandle);

	return true;
}

std::vector<float> BlurFilter::Impl::CalculateGaussianWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= max_blur_radius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

//
// Interface method
//

BlurFilter::BlurFilter():m_impl(new Impl())
{
}

BlurFilter::~BlurFilter()
{
	SAFE_DELETE(m_impl);
}

BlurFilter::BlurFilter(const BlurFilter&) {}
void BlurFilter::operator=(const BlurFilter&) {}

bool BlurFilter::Create(ID3D12Device* pDevice, UINT16 textureWidth, UINT16 textureHeight, DXGI_FORMAT textureFormat)
{
	IMPL.CreateWeights();
	IMPL.CreateRootSignature(pDevice);
	IMPL.CreatePSO(pDevice);
	IMPL.CreateResources(pDevice, textureWidth, textureHeight, textureFormat);
	IMPL.CreateDescriptors(pDevice);
	return true;
}

void BlurFilter::Blur(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* destTexture, UINT16 blurCount)
{
	pCmdList->SetPipelineState(IMPL.m_pso.Get());
	pCmdList->SetComputeRootSignature(IMPL.m_rootSig.Get());

	pCmdList->SetComputeRoot32BitConstants(0, 1, &IMPL.m_blurRadius, 0);
	pCmdList->SetComputeRoot32BitConstants(0, IMPL.m_weights.size(), IMPL.m_weights.data(), 1);

	// Transition to copy destination texture to blur's texture
	D12Helper::TransitionResourceState(pCmdList, IMPL.m_mainTex.Get(), 
		D3D12_RESOURCE_STATE_COMMON, 
		D3D12_RESOURCE_STATE_COPY_DEST);
	D12Helper::TransitionResourceState(pCmdList, destTexture, 
		D3D12_RESOURCE_STATE_COMMON, 
		D3D12_RESOURCE_STATE_COPY_SOURCE);

	pCmdList->CopyResource(IMPL.m_mainTex.Get(), destTexture);

	D12Helper::TransitionResourceState(pCmdList, IMPL.m_mainTex.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON);

	auto texDesc = IMPL.m_mainTex->GetDesc();
	UINT16 texWidth = texDesc.Width;
	UINT16 texHeight = texDesc.Height;

	for (uint16_t i = 0; i < blurCount; ++i)
	{
		// Transition blur's resources to get ready to compute blur
		D12Helper::TransitionResourceState(pCmdList, IMPL.m_mainTex.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
		D12Helper::TransitionResourceState(pCmdList, IMPL.m_subTex.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		CD3DX12_GPU_DESCRIPTOR_HANDLE heapHandle(IMPL.m_heap->GetGPUDescriptorHandleForHeapStart());
		heapHandle.Offset(3, IMPL.m_heapSize);
		pCmdList->SetComputeRootDescriptorTable(1, heapHandle);

		pCmdList->Dispatch(static_cast<UINT>(ceil(texWidth / 256.0f)), texHeight, 1);

		D12Helper::TransitionResourceState(pCmdList, IMPL.m_mainTex.Get(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE,
			D3D12_RESOURCE_STATE_COMMON);
		D12Helper::TransitionResourceState(pCmdList, IMPL.m_subTex.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_COMMON);
	}

	// Transition to copy blured texture back to destination resource
	D12Helper::TransitionResourceState(pCmdList, IMPL.m_subTex.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	D12Helper::TransitionResourceState(pCmdList, destTexture,
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);

	pCmdList->CopyResource(destTexture, IMPL.m_subTex.Get());

	D12Helper::TransitionResourceState(pCmdList, destTexture,
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COMMON);
	D12Helper::TransitionResourceState(pCmdList, IMPL.m_subTex.Get(),
		D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_COMMON);
}




