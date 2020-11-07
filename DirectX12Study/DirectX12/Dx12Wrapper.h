#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <vector>
#include <DirectXmath.h>
#include <memory>
#include <string>
#include <d3dx12.h>

class PMDModel;
using namespace Microsoft::WRL;

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
	ComPtr<ID3D12Device> dev_ = nullptr;
	ComPtr<ID3D12CommandAllocator> cmdAlloc_ = nullptr;
	ComPtr<ID3D12GraphicsCommandList> cmdList_ = nullptr;
	ComPtr<ID3D12CommandQueue> cmdQue_ = nullptr;
	ComPtr<IDXGIFactory7> dxgi_ = nullptr;
	ComPtr<IDXGISwapChain3> swapchain_ = nullptr;

	void CreateCommandFamily();
	void CreateSwapChain(const HWND& hwnd);

	// Renter Target View
	std::vector<ID3D12Resource*> backBuffers_;
	ID3D12DescriptorHeap* rtvHeap_;
	bool CreateRenderTargetViews();

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

	// Constant Buffer
	ID3D12Resource* transformBuffer_;
	ID3D12DescriptorHeap* transformDescHeap_;
	bool CreateTransformBuffer();

	// White Texture
	ID3D12Resource* whiteTexture_ = nullptr;
	ID3D12Resource* blackTexture_ = nullptr;
	ID3D12Resource* gradTexture_ = nullptr;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultColorTexture();

	std::vector<ID3D12Resource*> textureBuffers_;
	std::vector<ID3D12Resource*> sphBuffers_;
	std::vector<ID3D12Resource*> spaBuffers_;
	std::vector<ID3D12Resource*> toonBuffers_;
	// Create texture from PMD file
	bool CreateTextureFromFilePath(const std::wstring& path, ID3D12Resource*& buffer);
	void LoadTextureToBuffer();
	ID3D12Resource* materialBuffer_;
	ID3D12DescriptorHeap* materialDescHeap_;
	bool CreateMaterialAndTextureBuffer();

	void CreateRootSignature();
	bool CreateTexture(void);

	// Graphic pipeline
	ID3D12PipelineState* pipeline_ = nullptr;
	ID3D12RootSignature* rootSig_ = nullptr;
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

