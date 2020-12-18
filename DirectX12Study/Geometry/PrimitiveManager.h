#pragma once

#include <d3dx12.h>

class PrimitiveManager
{
public:
	PrimitiveManager();
	PrimitiveManager(ID3D12Device* pDevice);
	~PrimitiveManager();

	bool SetDevice(ID3D12Device* pDevice);
	bool SetWorldPassConstant(ID3D12Resource* pWorldPassConstant, size_t bufferSize);
	bool SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer);

	// Need to set up all resource for PMD Manager BEFORE initialize it
	bool Init(ID3D12GraphicsCommandList* cmdList);
private:
	PrimitiveManager(const PrimitiveManager&) = delete;
	PrimitiveManager& operator = (const PrimitiveManager&) = delete;

private:
	bool CreatePipeline();
	bool CreateRootSignature();
	bool CreatePipelineStateObject();

	bool m_isInitDone = false;
	// Device from engine
	ID3D12Device* m_device = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_rootSig = nullptr;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pipeline = nullptr;

	// World pass constant buffer binds only to VERTEX BUFFER
	// Descriptor heap stores descriptor of world pass constant buffer
	// Use for binding resource of engine to this pipeline
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_worldPCBHeap = nullptr;

	// Shadow depth buffer binds only to PIXEL SHADER
	// Descriptor heap stores descriptor of shadow depth buffer
	// Use for binding resource of engine to this pipeline
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_shadowDepthHeap = nullptr;
};

