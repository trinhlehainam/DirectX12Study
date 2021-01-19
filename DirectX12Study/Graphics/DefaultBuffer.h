#pragma once
#include <d3dx12.h>

// GPU read only buffer
class DefaultBuffer
{
public:
	DefaultBuffer() = default;
	~DefaultBuffer();

	ID3D12Resource* Resource();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();

	bool CreateBuffer(ID3D12Device* pDevice, size_t SyteInBytes);

	bool CreateTexture2D(ID3D12Device* pDevice, UINT64 width, UINT height, DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATES = D3D12_RESOURCE_STATE_GENERIC_READ,
		const D3D12_CLEAR_VALUE* = nullptr);

	void SetUpSubresource(const void* pData, LONG_PTR RowPitch, LONG_PTR SlicePitch);

	// Only use for NON-TEXTURE buffer
	bool SetUpSubresource(const void* pData, size_t SizeInBytes);

	// Update subresource to default buffer will be processed by GPU
	// Need set up this method at GPU time line
	bool UpdateSubresource(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList);

	// When subresource is updated to default buffer
	// It has no usage so clean it for better memeory usage
	// CAUTION :
	// - GPU need subresource to update data to default buffer
	// - Don't use this method until GPU done updating
	bool ClearSubresource();
private:

	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_intermedinateBuffer;
	D3D12_SUBRESOURCE_DATA* m_subresource = nullptr;
	bool m_isShaderResourceBuffer = false;
};

