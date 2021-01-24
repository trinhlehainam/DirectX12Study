#pragma once

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>

#include <dxgi1_6.h>
#include <Effekseer/Effekseer.h>
#include <EffekseerRendererDX12/EffekseerRendererDX12.h>

#include "../common.h"
#include "Camera.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "Material.h"
#include "Light.h"
#include "../Input/Keyboard.h"
#include "../Input/Mouse.h"
#include "../Utility/D12Helper.h"

constexpr uint16_t MAX_LIGHTS = 16;

struct WorldPassConstant {
	DirectX::XMFLOAT4X4 ViewProj;
	DirectX::XMFLOAT3 ViewPos;
	float padding0;
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 0.0f };

	DirectX::XMFLOAT4 FogColor;
	float FogStart;				// distance from eye to start seeing fog
	float FogRange;				// distance eye can see only fog
	float padding1;
	float padding2;

	Light Lights[MAX_LIGHTS];
};

using Microsoft::WRL::ComPtr;

class PrimitiveManager;
class PMDManager;
class TextureManager;
class PipelineManager;
class SpriteManager;
class BlurFilter;

/// <summary>
/// DirectX12 feature
/// </summary>
class D3D12App
{
public:
	D3D12App();
	~D3D12App();

	bool Initialize(const HWND&);
	
	// Update Direct3D12
	// true: no problem
	// false: error
	bool ProcessMessage();
	bool Update(const float& deltaTime);
	bool Render();
	void Terminate();
public:
	// function to receive keyboard input from Windows system
	void ClearKeyState();
	void OnWindowsKeyboardMessage(uint32_t msg, WPARAM keycode, LPARAM lparam);
	void OnWindowsMouseMessage(uint32_t msg, WPARAM keycode, LPARAM lparam);
private:
	Keyboard m_keyboard;
	Mouse m_mouse;
	Camera m_camera;
	DirectX::XMINT2 m_lastMousePos;

	void UpdateCamera(const float& deltaTime);
	void UpdateWorldPassConstant();
private:
	ComPtr<IDXGIFactory6> m_dxgi;
	ComPtr<IDXGISwapChain3> m_swapchain;
	
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12CommandQueue> m_cmdQue;
	ComPtr<ID3D12GraphicsCommandList> m_cmdList;
	ComPtr<ID3D12CommandAllocator> m_cmdAlloc;
	
	// Object helps CPU keep track of GPU process
	ComPtr<ID3D12Fence1> m_fence;

	// Current GPU target fence value use to check GPU is processing
	// If value of fence object haven't reach this target value
	// -> GPU is processing
	uint64_t m_targetFenceValue = 0;

	std::vector<FrameResource> m_frameResources;
	FrameResource* m_currentFrameResource = nullptr;
	const uint16_t num_frame_resources = 3;
	uint16_t m_currentFrameResourceIndex = 0;

	void CreateCommandFamily();
	void CreateSwapChain(const HWND& hwnd);
	void CreateFrameResources();

	// Renter Target View
	static constexpr unsigned int DEFAULT_BACK_BUFFER_COUNT = 2;
	static constexpr DXGI_FORMAT DEFAULT_BACK_BUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
	static constexpr DXGI_FORMAT DEFAULT_DEPTH_BUFFER_FORMAT = DXGI_FORMAT_D32_FLOAT;
	uint16_t m_currentBackBuffer = 0;
	std::vector<ComPtr<ID3D12Resource>> m_backBuffer;
	ComPtr<ID3D12DescriptorHeap> m_bbRTVHeap;
	bool CreateBackBufferView();

	// View depth buffer
	bool CreateViewDepthBuffer();
	ComPtr<ID3D12Resource> m_viewDepthBuffer;
	ComPtr<ID3D12DescriptorHeap> m_viewDSVHeap;

	// White Texture
	ComPtr<ID3D12Resource> m_whiteTexture;
	ComPtr<ID3D12Resource> m_blackTexture;
	ComPtr<ID3D12Resource> m_gradTexture ;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultTexture();

	std::unique_ptr<TextureManager> m_texMng;
	void CreateTextureManager();

	std::unique_ptr<PipelineManager> m_psoMng;
	void CreatePSOManager();
	void CreatePipelines();
	void CreateInputLayouts();
	void CreateRootSignatures();
	void CreatePSOs();

	void UpdateFence();
	void WaitForGPU();
private:
	// World Pass Constant
	UploadBuffer<WorldPassConstant> m_worldPCBuffer;
	bool CreateWorldPassConstant();
private:
	UpdateTextureBuffers m_updateBuffers;
	
	std::unique_ptr<PMDManager> m_pmdManager;
	void CreatePMDModel();

	std::unique_ptr<PrimitiveManager> m_primitiveManager;
	void CreatePrimitive();

	std::unique_ptr<BlurFilter> m_blurFilter;
	void CreateBlurFilter();

	std::unique_ptr<SpriteManager> m_spriteMng;
	void CreateSprite();
private:
	std::unordered_map<std::string, uint16_t> m_materialIndices;
	UploadBuffer<Material> m_materialCB;
	void CreateMaterials();

private:
	//
	// Post effect
	//

	void CreatePostEffect();
	void CreateRenderTargetTexture();
	void CreateBoardPolygonVertices();
	void CreateNormalMapTexture();
	void CreateBoardShadowDepthView();
	void CreateBoardViewDepthView();

	ComPtr<ID3D12Resource> m_rtTexture;
	ComPtr<ID3D12Resource> m_rtNormalTexture;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12DescriptorHeap> m_boardSRVHeap;

	// �؂�|�����_
	// TRIANGLESTRIP
	ComPtr<ID3D12Resource> m_boardPolyVert;
	D3D12_VERTEX_BUFFER_VIEW m_boardVBV;

	ComPtr<ID3D12Resource> m_normalMapTex;
	
private:
	// ShadowMapping
	bool CreateShadowDepthBuffer();

	ComPtr<ID3D12Resource> m_shadowDepthBuffer;
	ComPtr<ID3D12DescriptorHeap> m_shadowDSVHeap;

private:
	// Function for Render
	void RenderToShadowDepthBuffer();
	void RenderToRenderTargetTexture();
	void RenderToBackBuffer();

	void SetResourceStateForNextFrame();
	void SetBackBufferIndexForNextFrame();

private:
	// Effekseer
	EffekseerRenderer::Renderer* m_effekRenderer = nullptr;
	EffekseerRenderer::SingleFrameMemoryPool* m_effekPool = nullptr;
	Effekseer::Manager* m_effekManager = nullptr;
	EffekseerRenderer::CommandList* m_effekCmdList = nullptr;
	Effekseer::Effect* m_effekEffect = nullptr;
	Effekseer::Handle m_effekHandle = -1;

	void EffekseerInit();
	void EffekseerUpdate(const float& deltaTime);
	void EffekseerRender();
	void EffekseerTerminate();
};

