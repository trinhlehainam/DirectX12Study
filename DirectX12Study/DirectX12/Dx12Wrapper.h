#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXmath.h>
#include <memory>

class PMDModel;

/// <summary>
/// DirectX12 feature
/// </summary>
class Dx12Wrapper
{
private:
	std::shared_ptr<PMDModel> pmdModel_;

	struct BasicMatrix {
		DirectX::XMMATRIX world;
		DirectX::XMMATRIX viewproj;
	};
	struct BasicMaterial {
		DirectX::XMFLOAT3 diffuse;
		float alpha;
		DirectX::XMFLOAT3 specular;
		float specularity;
		DirectX::XMFLOAT3 ambient;
	};

	BasicMatrix* mappedBasicMatrix_ = nullptr;
	ID3D12Device* dev_ = nullptr;
	ID3D12CommandAllocator* cmdAlloc_ = nullptr;
	ID3D12GraphicsCommandList* cmdList_ = nullptr;
	ID3D12CommandQueue* cmdQue_ = nullptr;
	IDXGIFactory7* dxgi_ = nullptr;
	IDXGISwapChain3* swapchain_ = nullptr;

	// Renter Target View
	std::vector<ID3D12Resource*> bbResources_;
	ID3D12DescriptorHeap* rtvHeap_;
	bool CreateRenderTargetDescriptorHeap();

	// Depth/Stencil Buffer
	ID3D12Resource* depthBuffer_;
	ID3D12DescriptorHeap* dsvHeap_;
	bool CreateDepthBuffer();

	ID3D12Fence1* fence_ = nullptr;				// fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;

	// Vertex Buffer
	ID3D12Resource* vertexBuffer_;				//頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	// 頂点バッファを生成 (して、CPU側の頂点情報をコピー)
	void CreateVertexBuffer();

	ID3D12Resource* indicesBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;
	void CreateIndexBuffer();

	// Texture Buffer
	ID3D12Resource* textureBuffer_;
	ID3D12DescriptorHeap* resourceViewHeap_;		// Shader Resource View Desciptor Heap
	bool CreateShaderResource();

	// Constant Buffer
	ID3D12Resource* transformBuffer_;
	bool CreateTransformBuffer();

	ID3D12Resource* materialBuffer_;
	ID3D12DescriptorHeap* materialDescHeap_;
	bool CreateMaterialBuffer();

	// Root Signature
	void CreateRootSignature();

	std::vector<ID3D12Resource*> texturesBuffers_;
	bool CreateTextureFromPath(const std::wstring& path, ID3D12Resource*& buffer);
	bool CreateTexture();

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

