#pragma once

#include <unordered_map>
#include <string>
#include <d3dx12.h>
#include "GeometryCommon.h"
#include "Mesh.h"

class PrimitiveManager
{
public:
	PrimitiveManager();
	PrimitiveManager(ID3D12Device* pDevice);
	~PrimitiveManager();

	bool SetDevice(ID3D12Device* pDevice);
	bool SetWorldPassConstantGpuAddress(D3D12_GPU_VIRTUAL_ADDRESS worldPassConstantGpuAddress);
	bool SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer);
	bool SetViewDepth(ID3D12Resource* pViewDepthBuffer);

	bool Create(const std::string name, Geometry::Mesh primitive);
	// Need to set up all resource for PMD Manager BEFORE initialize it
	bool Init(ID3D12GraphicsCommandList* cmdList);

	bool ClearSubresources();
public:
	void Update(const float& deltaTime);
	void Render(ID3D12GraphicsCommandList* pCmdList);
	void RenderDepth(ID3D12GraphicsCommandList* pCmdList);
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

	D3D12_GPU_VIRTUAL_ADDRESS m_worldPassAdress = 0;

	// Descriptor heap stores descriptor of depth buffer
	// Use for binding resource of engine to this pipeline
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_depthHeap = nullptr;
	uint16_t m_depthBufferCount = 0;

private:
	Mesh<Geometry::Vertex> m_mesh;
	std::unordered_map<std::string, Geometry::Mesh> m_primitives;
};

