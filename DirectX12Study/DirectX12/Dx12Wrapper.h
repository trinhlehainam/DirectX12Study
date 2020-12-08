#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXmath.h>
#include <memory>
#include <string>

#include "../PMDLoader/PMDModel.h"
#include "../Input/Keyboard.h"
#include "../Input/Mouse.h"
#include "Dx12Helper.h"
#include "UploadBuffer.h"

using Microsoft::WRL::ComPtr;

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
	bool Render();
	void Terminate();
public:
	// function to receive keyboard input from Windows system
	void ClearKeyState();
	void OnKeyDown(uint8_t keycode);
	void OnKeyUp(uint8_t keycode);
public:
	void OnMouseMove(int x, int y);
	void OnMouseRightDown(int x, int y);
	void OnMouseRightUp(int x, int y);
	void OnMouseLeftDown(int x, int y);
	void OnMouseLeftUp(int x, int y);
private:
	Keyboard m_keyboard;
	Mouse m_mouse;
private:
	std::vector<std::shared_ptr<PMDModel>> pmdModelList_;

	ComPtr<ID3D12Device> dev_;
	ComPtr<ID3D12CommandAllocator> cmdAlloc_;
	ComPtr<ID3D12GraphicsCommandList> cmdList_;
	ComPtr<ID3D12CommandQueue> cmdQue_;
	ComPtr<IDXGIFactory6> dxgi_;
	ComPtr<IDXGISwapChain3> swapchain_;
	uint16_t currentBackBuffer_;

	ComPtr<ID3D12Fence1> fence_;				// fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;

	void CreateCommandFamily();
	void CreateSwapChain(const HWND& hwnd);

	// Renter Target View
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12DescriptorHeap> bbRTVHeap_;
	bool CreateBackBufferView();

	// Depth/Stencil Buffer
	ComPtr<ID3D12Resource> depthBuffer_;
	ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	bool CreateDepthBuffer();

	// White Texture
	ComPtr<ID3D12Resource> whiteTexture_;
	ComPtr<ID3D12Resource> blackTexture_;
	ComPtr<ID3D12Resource> gradTexture_ ;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultTexture();
private:

	// Send resouce from uploader(intermediate) buffer to GPU reading buffer
	void UpdateSubresourceToTextureBuffer(ID3D12Resource* texBuffer, D3D12_SUBRESOURCE_DATA& subresourcedata);

	void WaitForGPU();

	void CreatePMDModel();

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

	ComPtr<ID3D12Resource> normalMapTex_;
	void CreateNormalMapTexture();

	UploadBuffer<float> m_timeBuffer;
	void CreateTimeBuffer();

	ComPtr<ID3D12RootSignature> boardRootSig_;
	ComPtr<ID3D12PipelineState> boardPipeline_;
	void CreateBoardRootSignature();
	void CreateBoardPipeline();

	// ShadowMapping
	void CreateShadowMapping();

	ComPtr<ID3D12Resource> shadowDepthBuffer_;
	ComPtr<ID3D12DescriptorHeap> shadowDSVHeap_;
	ComPtr<ID3D12DescriptorHeap> shadowSRVHeap_;
	bool CreateShadowDepthBuffer();

	ComPtr<ID3D12PipelineState> shadowPipeline_;
	ComPtr<ID3D12RootSignature> shadowRootSig_;
	void CreateShadowRootSignature();
	void CreateShadowPipelineState();

	ComPtr<ID3D12Resource> planeVB_;
	D3D12_VERTEX_BUFFER_VIEW planeVBV_;
	ComPtr<ID3D12PipelineState> planePipeline_;
	ComPtr<ID3D12RootSignature> planeRootSig_;
	
	void CreatePlane();
	void CreatePlaneBuffer();
	void CreatePlaneRootSignature();
	void CreatePlanePipeLine();
	
	ComPtr<ID3D12DescriptorHeap> primitiveHeap_;
	void CreateDescriptorForPrimitive();

	// Function for Render
	void RenderToShadowDepthBuffer();
	void RenderToPostEffectBuffer();
	void RenderToBackBuffer();
	void RenderPrimitive();
	void SetResourceStateForNextFrame();
	void SetBackBufferIndexForNextFrame();
};

