#pragma once
#include <d3d12.h>

class GraphicsPSO
{
public:
	GraphicsPSO();
	~GraphicsPSO();

	void SetInputLayout(const D3D12_INPUT_LAYOUT_DESC& inputLayout);
	void SetInputLayout(UINT numElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDesc);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	void SetRenderTargetFormat(DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT,  UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetDepthStencilFormat(DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT, UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetRenderTargetFormats(UINT numRTVs, DXGI_FORMAT rtvFormat = DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT dsvFormat = DXGI_FORMAT_D32_FLOAT, UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetSampleMask(UINT sampleMask = D3D12_DEFAULT_SAMPLE_MASK);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC&);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC&);
	void SetBlendState(const D3D12_BLEND_DESC&);

	void SetVertexShader(const D3D12_SHADER_BYTECODE&);
	void SetPixelShader(const D3D12_SHADER_BYTECODE&);
	void SetDomainShader(const D3D12_SHADER_BYTECODE&);
	void SetHullShader(const D3D12_SHADER_BYTECODE&);
	void SetGeometryShader(const D3D12_SHADER_BYTECODE&);

	void SetRootSignature(ID3D12RootSignature* pRootSignature);

	bool Create(ID3D12Device* pDevice);
	ID3D12PipelineState* Get() const;
	void Reset();
private:
	// Don't allow copy semantics
	GraphicsPSO(const GraphicsPSO&);
	void operator = (const GraphicsPSO&);
private:
	class Impl;
	Impl* m_impl = nullptr;
};

