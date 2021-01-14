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
	ID3D12RootSignature* m_rootSig = nullptr;
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
	m_rootSig = nullptr;
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

bool RootSignature::AddRootParameterAsRootDescriptor(ROOT_DESCRIPTOR_TYPE rootDescriptor)
{
	CD3DX12_ROOT_PARAMETER param;
	
	switch (rootDescriptor)
	{
	case CONSTANT_BUFFER_VIEW:
		CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(param, IMPL.m_cbvIndex);
		IMPL.m_params.push_back(std::move(param));
		++IMPL.m_cbvIndex;
		break;
	case SHADER_RESOURCE_VIEW:
		break;
	case UNORDERED_ACCESS_VIEW:
		break;
	default:
		return false;
	}
	return true;
}

void RootSignature::AddStaticSampler()
{
	CD3DX12_STATIC_SAMPLER_DESC sampler(0,D3D12_FILTER_MIN_MAG_MIP_LINEAR);
	CD3DX12_STATIC_SAMPLER_DESC sampler1 = sampler;
	sampler1.ShaderRegister = 1;
	sampler1.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler1.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	sampler1.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	CD3DX12_STATIC_SAMPLER_DESC sampler2 = sampler;
	sampler2.ShaderRegister = 2;
	sampler2.Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;

	IMPL.m_samplers.reserve(3);
	IMPL.m_samplers.push_back(sampler);
	IMPL.m_samplers.push_back(sampler1);
	IMPL.m_samplers.push_back(sampler2);
}

bool RootSignature::Init(ID3D12Device* pDevice)
{
	CD3DX12_ROOT_SIGNATURE_DESC desc(
		IMPL.m_params.size(), IMPL.m_params.data(),
		IMPL.m_samplers.size(), IMPL.m_samplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	ComPtr<ID3DBlob> rootSigBlob = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	D12Helper::ThrowIfFailed(D3D12SerializeRootSignature(&desc,
		D3D_ROOT_SIGNATURE_VERSION_1_0, rootSigBlob.GetAddressOf(), 
		errorBlob.GetAddressOf())
	);
	D12Helper::OutputFromErrorBlob(errorBlob);
	D12Helper::ThrowIfFailed(pDevice->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), 
		rootSigBlob->GetBufferSize(),IID_PPV_ARGS(&IMPL.m_rootSig))
	);

	IMPL.m_ranges.clear();
	IMPL.m_samplers.clear();
	IMPL.m_params.clear();

	return true;
}

ID3D12RootSignature* RootSignature::Get()
{
	return IMPL.m_rootSig;
}

void RootSignature::SetEmpty()
{
	IMPL.m_rootSig = nullptr;
	IMPL.m_cbvIndex = 0;
	IMPL.m_srvIndex = 0;
	IMPL.m_uavIndex = 0;
	IMPL.m_samplerCnt = 0;
	IMPL.m_rangeIndex = 0;
}

RootSignature::RootSignature(const RootSignature&)
{
}

void RootSignature::operator=(const RootSignature&)
{
}
