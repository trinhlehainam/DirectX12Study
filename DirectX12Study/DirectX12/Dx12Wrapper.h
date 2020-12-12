#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXmath.h>
#include <memory>
#include <string>

#include "../Input/Keyboard.h"
#include "../Input/Mouse.h"
#include "Dx12Helper.h"
#include "UploadBuffer.h"
#include "../PMDModel/PMDManager.h"
#include "../common.h"


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

	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandAllocator> m_cmdAlloc;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<ID3D12CommandQueue> m_cmdQue;
	ComPtr<IDXGIFactory6> m_dxgi;
	ComPtr<IDXGISwapChain3> m_swapchain;
	
	// Object helps CPU keep track of GPU process
	ComPtr<ID3D12Fence1> m_fence;	

	// Current GPU target fence value use to check GPU is processing
	// If value of fence object haven't reach this target value
	// -> GPU is processing
	uint64_t m_targetFenceValue = 0;

	void CreateCommandFamily();
	void CreateSwapChain(const HWND& hwnd);

	// Renter Target View
	static constexpr unsigned int back_buffer_count = 2;
	uint16_t m_currentBackBuffer = 0;
	std::vector<ComPtr<ID3D12Resource>> m_backBuffer;
	ComPtr<ID3D12DescriptorHeap> m_bbRTVHeap;
	bool CreateBackBufferView();

	// Depth/Stencil Buffer
	ComPtr<ID3D12Resource> m_depthBuffer;
	ComPtr<ID3D12DescriptorHeap> m_dsvHeap;
	bool CreateDepthBuffer();

	// White Texture
	ComPtr<ID3D12Resource> m_whiteTexture;
	ComPtr<ID3D12Resource> m_blackTexture;
	ComPtr<ID3D12Resource> m_gradTexture ;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultTexture();

	void WaitForGPU();
private:
	// World Pass Constant
	UploadBuffer<WorldPassConstant> m_worldPCBuffer;
	ComPtr<ID3D12DescriptorHeap> m_worldPassConstantHeap;
	bool CreateWorldPassConstant();
private:
	UpdateTextureBuffers m_updateBuffers;

	std::unique_ptr<PMDManager> m_PMDmanager;
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

private:
	ComPtr<ID3D12Resource> planeVB_;
	D3D12_VERTEX_BUFFER_VIEW planeVBV_;
	ComPtr<ID3D12Resource> planeIB_;
	D3D12_INDEX_BUFFER_VIEW planeIBV_;
	ComPtr<ID3D12PipelineState> planePipeline_;
	ComPtr<ID3D12RootSignature> planeRootSig_;
	
	void CreatePlane();
	void CreatePlaneBuffer();
	void CreatePlaneVertexBuffer();
	void CreatePlaneIndexBuffer();
	void CreatePlaneRootSignature();
	void CreatePlanePipeLine();
	
	ComPtr<ID3D12DescriptorHeap> primitiveHeap_;
	void CreateDescriptorForPrimitive();

private:
	// Function for Render
	void RenderToShadowDepthBuffer();
	void RenderToPostEffectBuffer();
	void RenderToBackBuffer();
	void RenderPrimitive();
	void SetResourceStateForNextFrame();
	void SetBackBufferIndexForNextFrame();
};

