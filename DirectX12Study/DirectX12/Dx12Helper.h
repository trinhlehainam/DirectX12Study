#pragma once

#include <d3dx12.h>
#include <stdexcept>

using Microsoft::WRL::ComPtr;

class Dx12Helper
{
public:
	static ComPtr<ID3D12Resource> CreateBuffer(ComPtr<ID3D12Device>& device, size_t size, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_UPLOAD);

	// (width, height, DXGI_FORMAT, D3D12_RESOURCE_FLAGS) -> D3D12_RESOURCE_DESC
	static ComPtr<ID3D12Resource> CreateTex2DBuffer(ComPtr<ID3D12Device>& device, UINT64 width, UINT height,  DXGI_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM,
		D3D12_RESOURCE_FLAGS = D3D12_RESOURCE_FLAG_NONE, D3D12_HEAP_TYPE = D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATES = D3D12_RESOURCE_STATE_GENERIC_READ, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	static bool CreateDescriptorHeap(ComPtr<ID3D12Device>& device, ComPtr<ID3D12DescriptorHeap>& descriptorHeap, 
		UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible = false, UINT nodeMask = 0);

	static void OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob);

	static uint16_t AlignedValue(uint16_t value, uint16_t align);
	
	// Minimum memory block of constant buffer that hardware allows is 256 bits
	// Multiple constant buffers must be multiple memory blocks of 256 bits
	static uint16_t AlignedConstantBufferMemory(uint16_t byzeSize);

	static void ThrowIfFailed(HRESULT result);

	// Compile shader file at RUNTIME and convert it to bytecode for PSO (pipeline state object) use it
	// ->Return shader's bytecode
	static ComPtr<ID3DBlob> CompileShader(const wchar_t* filePath, const char* entryName, const char* targetVersion, D3D_SHADER_MACRO* defines = nullptr);

	// Return nullptr if FAILED to load texture from file
	static ComPtr<ID3D12Resource> CreateTextureFromFilePath(ComPtr<ID3D12Device>& device, const std::wstring& path);

private:
	class HrException : public std::runtime_error
	{
	public:
		HrException(HRESULT hr);
		HRESULT Error() const;
	private:
		HRESULT m_hr;
	};

	static std::string HrToString(HRESULT hr);
};



