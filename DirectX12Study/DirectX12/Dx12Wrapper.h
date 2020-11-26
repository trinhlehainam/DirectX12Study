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
private:
	std::shared_ptr<PMDModel> pmdModel_;
	std::shared_ptr<VMDMotion> vmdMotion_;

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
	ComPtr<IDXGIFactory6> dxgi_;
	ComPtr<IDXGISwapChain3> swapchain_;

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

	ComPtr<ID3D12Fence1> fence_;				// fence object ( necessary for cooperation between CPU and GPU )
	uint64_t fenceValue_ = 0;

	ComPtr<ID3D12Resource> CreateBuffer(size_t size, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_UPLOAD);

	ComPtr<ID3D12Resource> CreateTex2DBuffer(UINT64 width, UINT height, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_DEFAULT, DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES = D3D12_RESOURCE_STATE_GENERIC_READ, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	bool CreateDescriptorHeap(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, 
		bool isShaderVisible = false, UINT nodeMask = 0);

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

	// Bone buffer
	ComPtr<ID3D12Resource> boneBuffer_;
	DirectX::XMMATRIX* mappedBoneMatrix_ = nullptr;
	bool CreateBoneBuffer();

	void RecursiveCalculate(std::vector<PMDBone>& bones, std::vector<DirectX::XMMATRIX>& mats, size_t index);

	// White Texture
	ComPtr<ID3D12Resource> whiteTexture_;
	ComPtr<ID3D12Resource> blackTexture_;
	ComPtr<ID3D12Resource> gradTexture_ ;
	// If texture from file path is null, it will reference white texture
	void CreateDefaultTexture();

	// Send resouce from uploader(intermediate) buffer to GPU reading buffer
	void UpdateSubresourceToTextureBuffer(ID3D12Resource* texBuffer, D3D12_SUBRESOURCE_DATA& subresourcedata);

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

	void GPUCPUSync();
	void UpdateMotionTransform(const size_t& keyframe = 0);

	// Root-finding algorithm ( finding ZERO or finding ROOT )
	// There are 4 beizer points, but 2 of them are default at (0,0) and (127, 127)->(1,1) respectively
	float CalculateFromBezierByHalfSolve(float x, const DirectX::XMFLOAT2& p1, const DirectX::XMFLOAT2& p2, size_t n = 8);

	// Post effect rendering
	void CreatePostEffectTexture();

	ComPtr<ID3D12Resource> postEffectBuffer_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> passRTVHeap_ = nullptr;
	ComPtr<ID3D12DescriptorHeap> passSRVHeap_ = nullptr;
	void CreatePostEffectView();

	// ぺらポリ頂点
	// TRIANGLESTRIP
	ComPtr<ID3D12Resource> boardPolyVert_;
	D3D12_VERTEX_BUFFER_VIEW boardVBV_;
	void CreateBoardPolygonVertices();

	ComPtr<ID3D12RootSignature> boardRootSig_;
	ComPtr<ID3D12PipelineState> boardPipeline_;
	void CreateBoardRootSignature();
	void CreateBoardPipeline();

	ComPtr<ID3D12Resource> normalMapTex_;
public:
	bool Initialize(const HWND&);
	// Update Direct3D12
	// true: no problem
	// false: error
	bool Update();
	void Terminate();
};

