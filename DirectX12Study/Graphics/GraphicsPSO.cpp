#include "GraphicsPSO.h"

#include <vector>

#include "../Utility/D12Helper.h"

#define IMPL (*m_impl)

using Microsoft::WRL::ComPtr;

class GraphicsPSO::Impl
{
	friend GraphicsPSO;
private:
	Impl();
	~Impl();
private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_desc = {};
	ComPtr<ID3D12PipelineState> m_pso;
};

GraphicsPSO::Impl::Impl()
{

}

GraphicsPSO::Impl::~Impl()
{

}

/*
* Public method
*/

GraphicsPSO::GraphicsPSO():m_impl(new Impl())
{
}

GraphicsPSO::~GraphicsPSO()
{
	SAFE_DELETE(m_impl);
}

GraphicsPSO::GraphicsPSO(const GraphicsPSO&) {}

void GraphicsPSO::operator=(const GraphicsPSO&) {}

void GraphicsPSO::SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout)
{
	IMPL.m_desc.InputLayout = inputLayout;
}

void GraphicsPSO::SetInputElements(UINT numElements, const D3D12_INPUT_ELEMENT_DESC* pInput)
{
	IMPL.m_desc.InputLayout.NumElements = numElements;
	IMPL.m_desc.InputLayout.pInputElementDescs = pInput;
}

void GraphicsPSO::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology)
{
	IMPL.m_desc.PrimitiveTopologyType = primitiveTopology;
}

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, UINT MsaaCount, UINT MsaaQuality)
{
	IMPL.m_desc.NumRenderTargets = 1;
	IMPL.m_desc.RTVFormats[0] = rtvFormat;
	IMPL.m_desc.DSVFormat = dsvFormat;
	IMPL.m_desc.SampleDesc.Count = MsaaCount;
	IMPL.m_desc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetDepthStencilFormat(DXGI_FORMAT dsvFormat, UINT MsaaCount, UINT MsaaQuality)
{
	IMPL.m_desc.DSVFormat = dsvFormat;
	IMPL.m_desc.SampleDesc.Count = MsaaCount;
	IMPL.m_desc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetRenderTargetFormats(UINT numRTVs, DXGI_FORMAT rtvFormat, DXGI_FORMAT dsvFormat, UINT MsaaCount, UINT MsaaQuality)
{
	IMPL.m_desc.NumRenderTargets = numRTVs;
	for(UINT i = 0; i < numRTVs; ++i)
		IMPL.m_desc.RTVFormats[i] = rtvFormat;
	IMPL.m_desc.DSVFormat = dsvFormat;
	IMPL.m_desc.SampleDesc.Count = MsaaCount;
	IMPL.m_desc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetSampleMask(UINT sampleMask)
{
	IMPL.m_desc.SampleMask = sampleMask;
}

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& rasterizerDesc)
{
	IMPL.m_desc.RasterizerState = rasterizerDesc;
}

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& depthstencilDesc)
{
	IMPL.m_desc.DepthStencilState = depthstencilDesc;
}

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& blendDesc)
{
	IMPL.m_desc.BlendState = blendDesc;
}

void GraphicsPSO::SetVertexShader(const D3D12_SHADER_BYTECODE& vs)
{
	IMPL.m_desc.VS = vs;
}

void GraphicsPSO::SetPixelShader(const D3D12_SHADER_BYTECODE& ps)
{
	IMPL.m_desc.PS = ps;
}

void GraphicsPSO::SetDomainShader(const D3D12_SHADER_BYTECODE& ds)
{
	IMPL.m_desc.DS = ds;
}

void GraphicsPSO::SetHullShader(const D3D12_SHADER_BYTECODE& hs)
{
	IMPL.m_desc.HS = hs;
}

void GraphicsPSO::SetGeometryShader(const D3D12_SHADER_BYTECODE& gs)
{
	IMPL.m_desc.GS = gs;
}

void GraphicsPSO::SetRootSignature(ID3D12RootSignature* pRootSignature)
{
	IMPL.m_desc.pRootSignature = pRootSignature;
}

bool GraphicsPSO::Create(ID3D12Device* pDevice)
{
	auto result = pDevice->CreateGraphicsPipelineState(&IMPL.m_desc, IID_PPV_ARGS(IMPL.m_pso.ReleaseAndGetAddressOf()));
	return SUCCEEDED(result);
}

ID3D12PipelineState* GraphicsPSO::Get() const
{
	return IMPL.m_pso.Get();
}

void GraphicsPSO::Reset()
{
	IMPL.m_pso.Reset();
	IMPL.m_desc = {};
}





