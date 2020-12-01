#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXmath.h>
#include <memory>
#include <string>
#include <d3dx12.h>
#include "../PMDLoader/PMDModel.h"

using Microsoft::WRL::ComPtr;

class VMDMotion;

/// <summary>
/// DirectX12 feature
/// </summary>
class Dx12Wrapper
{
public:
	bool Initialize(const HWND&);
	// Update Direct3D12
	// true: no problem
	// false: error
	bool Update(const float& deltaTime);
	void Terminate();
private:
	std::vector<std::shared_ptr<PMDModel>> pmdModelList_;

	ComPtr<ID3D12Device> dev_;
	ComPtr<ID3D12CommandAllocator> cmdAlloc_;
	ComPtr<ID3D12GraphicsCommandList> cmdList_;
	ComPtr<ID3D12CommandQueue> cmdQue_;
	ComPtr<IDXGIFactory6> dxgi_;
	ComPtr<IDXGISwapChain3> swapchain_;

	ComPtr<ID3D12Fence1> fence_;				// fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;

	void CreateCommandFamily();
	void CreateSwapChain(const HWND& hwnd);

	// Renter Target View
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12DescriptorHeap> bbRTVHeap_;
	bool CreateRenderTargetViews();

	// Depth/Stencil Buffer
	ComPtr<ID3D12Resource> depthBuffer_;
	ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	bool CreateDepthBuffer();

	// Depth Buffer for Shadow Map
	void CreateShadowMapping();

	ComPtr<ID3D12Resource> shadowDepthBuffer_;
	ComPtr<ID3D12DescriptorHeap> shadowDSVHeap_;
	ComPtr<ID3D12DescriptorHeap> shadowSRVHeap_;
	bool CreateShadowDepthBuffer();

	ComPtr<ID3D12PipelineState> shadowPipeline_;
	ComPtr<ID3D12RootSignature> shadowRootSig_;
	void CreateShadowRootSignature();
	void CreateShadowPipelineState();

	void DrawShadow();

	ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_UPLOAD);

	ComPtr<ID3D12Resource> CreateTex2DBuffer(UINT64 width, UINT height, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_DEFAULT, DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES = D3D12_RESOURCE_STATE_GENERIC_READ, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	bool CreateDescriptorHeap(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, 
		bool isShaderVisible = false, UINT nodeMask = 0);

	// White Texture
	ComPtr<ID3D12Resource> whiteTexture_;
	ComPtr<ID3D12Resource> blackTexture_;
	ComPtr<ID3D12Resource> gradTexture_ ;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultTexture();

	// Send resouce from uploader(intermediate) buffer to GPU reading buffer
	void UpdateSubresourceToTextureBuffer(ID3D12Resource* texBuffer, D3D12_SUBRESOURCE_DATA& subresourcedata);

	// Create texture from PMD file
	bool CreateTextureFromFilePath(const std::wstring& path, ComPtr<ID3D12Resource>& buffer);

	void GPUCPUSync();

	void OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob);

	// Post effect rendering
	void CreatePostEffectTexture();

	ComPtr<ID3D12Resource> rtTexture_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> passRTVHeap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> passSRVHeap_ = nullptr;
	void CreateViewForRenderTargetTexture();

	// ‚Ø‚çƒ|ƒŠ’¸“_
	// TRIANGLESTRIP
	ComPtr<ID3D12Resource> boardPolyVert_;
	D3D12_VERTEX_BUFFER_VIEW boardVBV_;
	void CreateBoardPolygonVertices();

	ComPtr<ID3D12RootSignature> boardRootSig_;
	ComPtr<ID3D12PipelineState> boardPipeline_;
	void CreateBoardRootSignature();
	void CreateBoardPipeline();

	ComPtr<ID3D12Resource> normalMapTex_;
	void CreateNormalMapTexture();

	float* time_ = nullptr;
	ComPtr<ID3D12Resource> timeBuffer_;
	void CreateTimeBuffer();
};

