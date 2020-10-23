#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>

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

	std::vector<ID3D12Resource*> bbResources_;
	ID3D12DescriptorHeap* rtvHeap_;
	bool CreateRenderTargetDescriptorHeap();

	ID3D12Fence1* fence_ = nullptr;				// fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;

	// Vertex Buffer
	ID3D12Resource* vertexBuffer_;				//���_�o�b�t�@
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	// ���_�o�b�t�@�𐶐� (���āACPU���̒��_�����R�s�[)
	void CreateVertexBuffer();

	// Texture Buffer
	ID3D12Resource* textureBuffer_;
	ID3D12DescriptorHeap* resourceViewHeap_;		// Shader Resource View Desciptor Heap
	bool CreateTexure();

	// Constant Buffer
	ID3D12Resource* constantBuffer_;
	bool CreateConstantBuffer();

	// Graphic pipeline
	ID3D12PipelineState* pipeline_ = nullptr;
	ID3D12RootSignature* rootSig_ = nullptr;
	void OutputFromErrorBlob(ID3DBlob* errBlob);
	bool CreatePipelineState();
public:
	bool Initialize(HWND);
	// Update Direct3D12
	// true: no problem
	// false: error
	bool Update();
	void Terminate();
};

