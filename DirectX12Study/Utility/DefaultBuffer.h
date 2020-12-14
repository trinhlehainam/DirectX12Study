#pragma once
#include <d3dx12.h>

using Microsoft::WRL::ComPtr;

// GPU read only buffer
class DefaultBuffer
{
public:
	DefaultBuffer() = default;
	~DefaultBuffer();

	ID3D12Resource* Resource();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();

	void SetUpSubresource(const void* pData, LONG_PTR RowPitch, LONG_PTR SlicePitch,
		bool IsShaderResourceBuffer = false);

	// Only use for NON-TEXTURE buffer
	void SetUpSubresource(const void* pData, size_t SizeInBytes);

	// Update subresource to default buffer will be processed by GPU
	// Need set up this method at GPU 
	bool UpdateSubresource(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList);

	// When subresource is updated to default buffer
	// It has no usage so clean it for better memeory usage
	// CAUTION :
	// - GPU need subresource to update data to default buffer
	// - Don't use this method until GPU done updating
	bool ClearSubresource();
private:
	ComPtr<ID3D12Resource> m_buffer = nullptr;
	ComPtr<ID3D12Resource> m_intermedinateBuffer = nullptr;
	D3D12_SUBRESOURCE_DATA* m_subresource = nullptr;
	bool m_isShaderResourceBuffer = false;
};

