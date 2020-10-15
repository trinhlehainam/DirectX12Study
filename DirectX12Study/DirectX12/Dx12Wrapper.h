#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

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
	ID3D12CommandList* cmdList_ = nullptr;
	ID3D12CommandQueue* cmdQue_ = nullptr;
	IDXGIFactory7* dxgi_ = nullptr;
	IDXGISwapChain* swapchain_ = nullptr;
public:
	bool Initialize(HWND);
	void Terminate();
};

