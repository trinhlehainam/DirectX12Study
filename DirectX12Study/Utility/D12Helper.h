#pragma once

#include <d3dx12.h>
#include <stdexcept>
#include <unordered_map>

namespace D12Helper
{

	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBuffer(ID3D12Device* pDevice, size_t sizeInBytes, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_UPLOAD);

	// (width, height, DXGI_FORMAT, D3D12_RESOURCE_FLAGS) -> D3D12_RESOURCE_DESC
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTexture2D(ID3D12Device* pdevice, UINT64 width, UINT height,  DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATES = D3D12_RESOURCE_STATE_GENERIC_READ, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	bool CreateDescriptorHeap(ID3D12Device* pDevice, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& pDescriptorHeap,
		UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible = false, UINT nodeMask = 0);

	void OutputFromErrorBlob(Microsoft::WRL::ComPtr<ID3DBlob>& errBlob);

	 size_t AlignedValue(size_t value, size_t align);
	
	// Minimum memory block of constant buffer that hardware allows is 256 bits
	// Multiple constant buffers must be multiple memory blocks of 256 bits
	size_t AlignedConstantBufferMemory(size_t byzeSize);

	void ThrowIfFailed(HRESULT result);

	// Compile shader file at RUNTIME and convert it to bytecode for PSO (pipeline state object) use it
	// ->Return shader's bytecode
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShaderFromFile(const wchar_t* filePath, const char* entryName, const char* targetVersion, D3D_SHADER_MACRO* defines = nullptr);

	// Return nullptr if FAILED to load texture from file
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureFromFilePath(Microsoft::WRL::ComPtr<ID3D12Device>& device, const std::wstring& path);

	// Create resource has default heap type (GPU read only)
	// Use upload buffer to UPDATE resource to default buffer
	// *** UPLOAD BUFFER SHOULD BE EMPTY
	// *** NEED TO KEEP UPLOAD BUFFER ALIVE UNTIL GPU UPDATE DATA 
	//  FROM UPLOAD BUFFER TO DEFAULT BUFFER
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& emptyUploadBuffer, const void* pData, size_t dataSize);

	// *** NEED TO KEEP UPLOAD BUFFER ALIVE UNTIL GPU UPDATE DATA 
	//  FROM UPLOAD BUFFER TO DEFAULT BUFFER
	bool UpdateDataToTextureBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList,
		Microsoft::WRL::ComPtr<ID3D12Resource>& textureBuffer, Microsoft::WRL::ComPtr<ID3D12Resource>& emptyUploadBuffer, const D3D12_SUBRESOURCE_DATA& subResource);

	// Change resource state at GPU executing time
	void ChangeResourceState(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, 
		D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

	class HrException : public std::runtime_error
	{
	public:
		HrException(HRESULT hr);
		HRESULT Error() const;
	private:
		HRESULT m_hr;
		friend std::string HrToString(HRESULT hr);
	};

	std::string HrToString(HRESULT hr);
};

// Buffers use for update to texture buffer
class UpdateTextureBuffers
{
public:
	UpdateTextureBuffers() = default;

	bool Add(const std::string& bufferName, const void* pData, LONG_PTR rowPitch, LONG_PTR slicePitch);
	bool Clear();
	const D3D12_SUBRESOURCE_DATA& GetSubresource(const std::string& bufferName);
	Microsoft::WRL::ComPtr<ID3D12Resource>& GetBuffer(const std::string& bufferName);
private:
	UpdateTextureBuffers(const UpdateTextureBuffers&) = delete;
	UpdateTextureBuffers& operator = (const UpdateTextureBuffers&) = delete;
private:
	using UpdateBuffer_t = std::pair<Microsoft::WRL::ComPtr<ID3D12Resource>, D3D12_SUBRESOURCE_DATA>;
	std::unordered_map<std::string, UpdateBuffer_t> m_updateBuffers;
};

#define SAFE_RELEASE(x)\
if(x != nullptr)\
{\
x->Release();\
x = nullptr; \
}

#define SAFE_DELETE(x)\
if(x != nullptr)\
{\
delete x;\
x = nullptr; \
}

#define SAFE_DELETE_ARRAY(x)\
if(x != nullptr)\
{\
delete[] x;\
x = nullptr; \
}