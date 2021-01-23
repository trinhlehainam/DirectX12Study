#include "RootSignature.h"

#include <vector>
#include <unordered_map>

#include "../Utility/D12Helper.h"

#define IMPL (*m_impl)

using namespace Microsoft::WRL;

class RootSignature::Impl
{
	friend RootSignature;
private:
	Impl();
	~Impl();
private:
	ComPtr<ID3D12RootSignature> m_rootSig;
	std::unordered_map<uint16_t,std::vector<CD3DX12_DESCRIPTOR_RANGE>> m_ranges;
	std::vector<CD3DX12_ROOT_PARAMETER> m_params;
	std::vector<CD3DX12_STATIC_SAMPLER_DESC> m_samplers;
	uint16_t m_cbvIndex = 0;
	uint16_t m_srvIndex = 0;
	uint16_t m_uavIndex = 0;
	uint16_t m_samplerCnt = 0;
	uint16_t m_rangeIndex = 0;
};

RootSignature::Impl::Impl()
{

}

RootSignature::Impl::~Impl()
{

}

RootSignature::RootSignature():m_impl(new Impl())
{

}

RootSignature::~RootSignature()
{
	SAFE_DELETE(m_impl);
}

void RootSignature::AddRootParameterAsDescriptorTable(UINT16 numCBV, UINT16 numSRV, UINT16 numUAV, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	const uint16_t num_ranges = numCBV + numSRV + numUAV;

	auto& ranges = IMPL.m_ranges[IMPL.m_rangeIndex];
	
	ranges.reserve(num_ranges);
	if(numCBV > 0)
		ranges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, numCBV, IMPL.m_cbvIndex));
	if(numSRV > 0)
		ranges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, numSRV, IMPL.m_srvIndex));
	if(numUAV > 0)
		ranges.push_back(CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, numUAV, IMPL.m_uavIndex));

	CD3DX12_ROOT_PARAMETER param = {};
	CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(param, ranges.size(), ranges.data(), shaderVisibility);
	IMPL.m_params.push_back(std::move(param));

	++IMPL.m_rangeIndex;
	IMPL.m_cbvIndex += numCBV;
	IMPL.m_srvIndex += numSRV;
	IMPL.m_uavIndex += numUAV;
}

bool RootSignature::AddRootParameterAsDescriptor(ROOT_DESCRIPTOR_TYPE rootDescriptor, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	CD3DX12_ROOT_PARAMETER param;
	
	switch (rootDescriptor)
	{
	case CBV:
		CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(param, IMPL.m_cbvIndex, shaderVisibility);
		IMPL.m_params.push_back(std::move(param));
		++IMPL.m_cbvIndex;
		break;
	case SRV:
		break;
	case UAV:
		break;
	default:
		return false;
	}
	return true;
}

bool RootSignature::AddRootParameterAs32BitsConstants(UINT16 num32BitsConstants, D3D12_SHADER_VISIBILITY shaderVisibility)
{
	CD3DX12_ROOT_PARAMETER param;
	param.InitAsConstants(num32BitsConstants, IMPL.m_cbvIndex, shaderVisibility);
	IMPL.m_params.push_back(std::move(param));
	++IMPL.m_cbvIndex;

	return true;
}

void RootSignature::AddStaticSampler(STATIC_SAMPLER_TYPE type)
{
	switch (type)
	{
	case STATIC_SAMPLER_TYPE::LINEAR_WRAP:
		IMPL.m_samplers.push_back(
			CD3DX12_STATIC_SAMPLER_DESC(IMPL.m_samplerCnt, D3D12_FILTER_MIN_MAG_MIP_LINEAR));
		++IMPL.m_samplerCnt;
		break;
	case STATIC_SAMPLER_TYPE::LINEAR_CLAMP:
		IMPL.m_samplers.push_back(
			CD3DX12_STATIC_SAMPLER_DESC(IMPL.m_samplerCnt, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP, 
				D3D12_TEXTURE_ADDRESS_MODE_CLAMP
				));
		++IMPL.m_samplerCnt;
		break;
	case STATIC_SAMPLER_TYPE::LINEAR_BORDER:
		IMPL.m_samplers.push_back(
			CD3DX12_STATIC_SAMPLER_DESC(IMPL.m_samplerCnt, D3D12_FILTER_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,
				D3D12_TEXTURE_ADDRESS_MODE_BORDER
			));
		++IMPL.m_samplerCnt;
		break;
	case STATIC_SAMPLER_TYPE::COMPARISION_LINEAR_WRAP:
		IMPL.m_samplers.push_back(
			CD3DX12_STATIC_SAMPLER_DESC(IMPL.m_samplerCnt, D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR,
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,
				D3D12_TEXTURE_ADDRESS_MODE_BORDER,
				D3D12_TEXTURE_ADDRESS_MODE_BORDER
			));
		++IMPL.m_samplerCnt;
		break;
	default:
		break;
	}
}

bool RootSignature::Create(ID3D12Device* pDevice)
{
	CD3DX12_ROOT_SIGNATURE_DESC desc(
		IMPL.m_params.size(), IMPL.m_params.data(),
		IMPL.m_samplers.size(), IMPL.m_samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob;
	ComPtr<ID3DBlob> errorBlob;
	D12Helper::ThrowIfFailed(D3D12SerializeRootSignature(&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0, rootSigBlob.GetAddressOf(), 
		errorBlob.GetAddressOf())
	);
	D12Helper::OutputFromErrorBlob(errorBlob);
	D12Helper::ThrowIfFailed(pDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), 
		rootSigBlob->GetBufferSize(),IID_PPV_ARGS(IMPL.m_rootSig.ReleaseAndGetAddressOf()))
	);

	return true;
}

ID3D12RootSignature* RootSignature::Get() const
{
	return IMPL.m_rootSig.Get();
}

void RootSignature::Reset()
{
	IMPL.m_rootSig.Reset();
	IMPL.m_cbvIndex = 0;
	IMPL.m_srvIndex = 0;
	IMPL.m_uavIndex = 0;
	IMPL.m_samplerCnt = 0;
	IMPL.m_rangeIndex = 0;
	IMPL.m_ranges.clear();
	IMPL.m_samplers.clear();
	IMPL.m_params.clear();
}

RootSignature::RootSignature(const RootSignature&)
{
}

void RootSignature::operator=(const RootSignature&)
{
}
