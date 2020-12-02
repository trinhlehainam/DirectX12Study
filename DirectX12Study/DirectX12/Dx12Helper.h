#include <d3dx12.h>

using Microsoft::WRL::ComPtr;

class Dx12Helper
{
public:
	static ComPtr<ID3D12Resource> CreateBuffer(ComPtr<ID3D12Device>& device, size_t size, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_UPLOAD);

	static ComPtr<ID3D12Resource> CreateTex2DBuffer(ComPtr<ID3D12Device>& device, UINT64 width, UINT height, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_DEFAULT, DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES = D3D12_RESOURCE_STATE_GENERIC_READ, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	static bool CreateDescriptorHeap(ComPtr<ID3D12Device>& device, ComPtr<ID3D12DescriptorHeap>& descriptorHeap, 
		UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible = false, UINT nodeMask = 0);

	static void OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob);
};