#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

struct Size
{
	size_t width;
	size_t height;
};

/// <summary>
/// DirectX12 feature
/// </summary>
class Dx12Wrapper
{
private:
	ID3D12Device* dev_ = nullptr;
	ID3D12CommandAllocator* cmdAlloc_ = nullptr;
	ID3D12GraphicsCommandList* cmdList_ = nullptr;
	ID3D12CommandQueue* cmdQue_ = nullptr;
	IDXGIFactory7* dxgi_ = nullptr;
	IDXGISwapChain3* swapchain_ = nullptr;

	std::vector<ID3D12Resource*> bbResources_; // 
	ID3D12DescriptorHeap* rtvHeap_;
	bool CreateRenderTargetDescriptorHeap();

	ID3D12Fence1* fence_ = nullptr; // fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;
public:
	bool Initialize(HWND);
	// Update Direct3D12
	// true: no problem
	// false: error
	bool Update();
	void Terminate();
};

