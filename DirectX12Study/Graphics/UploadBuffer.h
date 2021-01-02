#pragma once
#include <d3dx12.h>

// Manage resource type upload and its data
template<typename T>
class UploadBuffer
{
public:
	UploadBuffer() = default;
	explicit UploadBuffer(ID3D12Device* device, uint32_t elementCount = 1, bool isConstantBuffer = false);
	explicit UploadBuffer(UploadBuffer&& other) noexcept;
	~UploadBuffer();

	bool Create(ID3D12Device* device, uint32_t elementCount = 1, bool isConstantBuffer = false);
	ID3D12Resource* Get();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress(uint32_t index = 0);

	size_t SizeInBytes() const;
	size_t ElementSize() const;

	/// <summary>
	/// <para>If buffer has MULTIPLE elements of data, get data of index element</para>
	/// If buffer has only one element , index is automatically set to default (0)
	/// </summary>
	/// <param name="index: "></param>
	/// <returns>
	/// <para>pointer to mapped data</para>
	/// nullptr if Copy is Disable
	/// </returns>
	T* GetHandleMappedData(uint32_t index = 0);

	// If buffer has MULTIPLE elements, copy data to specific index
	// return false if index is invalid (index is larger than number of element)
	bool CopyData(const T& data, uint32_t index = 0);

	// Copy an array data to buffer
	// return false if elementCount is zero (0) 
	// or different with number of element
	bool CopyData(const T* pArrayData, uint32_t elementCount = 1);

	// return false if the size of array is empty 
	// or different with number of element
	bool CopyData(const std::vector<T>& arrayData);

	// return false if it's already in copyable state
	void EnableCopy();
	// return false if it's already in disable copy state
	void DisableCopy();

	// Take all resource of other upload buffer
	void Move(UploadBuffer& other);
private:
	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator = (const UploadBuffer&) = delete;

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_buffer = nullptr;
	// Pointer to multiple one-byte to do padding
	uint8_t* m_mappedData = nullptr;
	// uint16_t max value is 0xffff (65535)
	// it maybe too small with size of Indices
	uint32_t m_elementCount = 0;
	bool m_isConstantBuffer = false;
	bool m_isCopyable = true;
};

#include "UploadBuffer.inl"



