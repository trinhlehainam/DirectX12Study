#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXmath.h>
#include <memory>
#include <string>
#include <d3dx12.h>

class PMDModel;
using Microsoft::WRL::ComPtr;

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
	ComPtr<ID3D12Device> dev_;
	ComPtr<ID3D12CommandAllocator> cmdAlloc_;
	ComPtr<ID3D12GraphicsCommandList> cmdList_;
	ComPtr<ID3D12CommandQueue> cmdQue_;
	ComPtr<IDXGIFactory7> dxgi_;
	ComPtr<IDXGISwapChain3> swapchain_;

	void CreateCommandFamily();
	void CreateSwapChain(const HWND& hwnd);

	// Renter Target View
	std::vector<ComPtr<ID3D12Resource>> backBuffers_;
	ComPtr<ID3D12DescriptorHeap> rtvHeap_;
	bool CreateRenderTargetViews();

	// Depth/Stencil Buffer
	ComPtr<ID3D12Resource> depthBuffer_;
	ComPtr<ID3D12DescriptorHeap> dsvHeap_;
	bool CreateDepthBuffer();

	ComPtr<ID3D12Fence1> fence_;				// fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;

	ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_HEAP_PROPERTIES = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD));

	// Vertex Buffer
	ComPtr<ID3D12Resource> vertexBuffer_;				//頂点バッファ
	D3D12_VERTEX_BUFFER_VIEW vbView_;
	// 頂点バッファを生成 (して、CPU側の頂点情報をコピー)
	void CreateVertexBuffer();

	ComPtr<ID3D12Resource> indicesBuffer_;
	D3D12_INDEX_BUFFER_VIEW ibView_;
	void CreateIndexBuffer();

	// Constant Buffer
	ComPtr<ID3D12Resource> transformBuffer_;
	ComPtr<ID3D12DescriptorHeap> transformDescHeap_;
	bool CreateTransformBuffer();

	// White Texture
	ComPtr<ID3D12Resource> whiteTexture_;
	ComPtr<ID3D12Resource> blackTexture_;
	ComPtr<ID3D12Resource> gradTexture_ ;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultColorTexture();

	std::vector<ComPtr<ID3D12Resource>> textureBuffers_;
	std::vector<ComPtr<ID3D12Resource>> sphBuffers_;
	std::vector<ComPtr<ID3D12Resource>> spaBuffers_;
	std::vector<ComPtr<ID3D12Resource>> toonBuffers_;
	// Create texture from PMD file
	bool CreateTextureFromFilePath(const std::wstring& path, ComPtr<ID3D12Resource>& buffer);
	void LoadTextureToBuffer();
	ComPtr<ID3D12Resource> materialBuffer_;
	ComPtr<ID3D12DescriptorHeap> materialDescHeap_;
	bool CreateMaterialAndTextureBuffer();

	void CreateRootSignature();
	bool CreateTexture(void);

	// Graphic pipeline
	ComPtr<ID3D12PipelineState> pipeline_;
	ComPtr<ID3D12RootSignature> rootSig_ ;
	void OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob);
	bool CreatePipelineStateObject();
public:
	bool Initialize(const HWND&);
	// Update Direct3D12
	// true: no problem
	// false: error
	bool Update();
	void Terminate();
};

