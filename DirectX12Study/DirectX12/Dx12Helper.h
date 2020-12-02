#include <d3dx12.h>
#include <stdexcept>

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

	static uint16_t AlignedValue(uint16_t value, uint16_t align);
	
	static uint16_t AlignedConstantBufferMemory(uint16_t byzeSize);

	static void ThrowIfFailed(HRESULT result);
};

inline std::string HrToString(HRESULT hr)
{
	char s_str[64] = {};
	sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
	return std::string(s_str);
}

class HrException : public std::runtime_error
{
public:
	HrException(HRESULT hr);
	HRESULT Error() const;
private:
	HRESULT m_hr;
};