#pragma once
#include <vector>
#include <cassert>
#include "Dx12Helper.h"

using Microsoft::WRL::ComPtr;

// Manage resource type upload and its data
template<typename T>
class UploadBuffer
{
public:
	UploadBuffer() = default;
	UploadBuffer(ComPtr<ID3D12Device>& device, uint32_t elementCount = 1, bool isConstantBuffer = false);
	~UploadBuffer();

	bool Create(ID3D12Device* device, uint32_t elementCount = 1, bool isConstantBuffer = false);
	ID3D12Resource* Resource();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();

	size_t SizeInBytes() const;
	size_t ElementSize() const;

	// If buffer has MULTIPLE elements of data, get data of input index
	// If buffer has only one element , index is automatically set to default (0)
	T& MappedData(uint32_t index = 0);

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
	bool EnableCopy();
	// return false if it's already in disable copy state
	bool DisableCopy();
private:
	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator = (const UploadBuffer&) = delete;

private:
	ComPtr<ID3D12Resource> m_buffer = nullptr;
	T* m_mappedData = nullptr;
	// uint16_t max value is 0xffff (65535)
	// it maybe too small with size of Indices
	uint32_t m_elementCount = 0;
	bool m_isConstantBuffer = false;
	bool m_isCopyable = true;
};

template<typename T>
inline UploadBuffer<T>::UploadBuffer(ComPtr<ID3D12Device>& device, uint32_t elementCount, bool isConstantBuffer)
{
	m_elementCount = elementCount;
	m_isConstantBuffer = isConstantBuffer;
	
	auto bufferSize = isConstantBuffer ? Dx12Helper::AlignedConstantBufferMemory(sizeof(T)) : sizeof(T);
	bufferSize *= m_elementCount;

	m_buffer = Dx12Helper::CreateBuffer(device, bufferSize);
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));
}

template<typename T>
inline UploadBuffer<T>::~UploadBuffer()
{
	if (m_buffer != nullptr && m_isCopyable)
		m_buffer->Unmap(0, nullptr);
	m_mappedData = nullptr;
}

template<typename T>
inline bool UploadBuffer<T>::Create(ID3D12Device* pDevice, uint32_t elementCount, bool isConstantBuffer)
{
	m_elementCount = elementCount;
	m_isConstantBuffer = isConstantBuffer;

	m_buffer = Dx12Helper::CreateBuffer(pDevice, SizeInBytes());
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));

	return false;
}

template<typename T>
inline ID3D12Resource* UploadBuffer<T>::Resource()
{
	return m_buffer.Get();
}

template<typename T>
inline D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<T>::GetGPUVirtualAddress()
{
	assert(m_buffer.Get() != nullptr);
	return m_buffer.Get()->GetGPUVirtualAddress();
}

template<typename T>
inline size_t UploadBuffer<T>::SizeInBytes() const
{
	return ElementSize() * m_elementCount;
}

template<typename T>
inline size_t UploadBuffer<T>::ElementSize() const
{
	return m_isConstantBuffer ?
		Dx12Helper::AlignedConstantBufferMemory(sizeof(T))
		: sizeof(T);
}

template<typename T>
inline T& UploadBuffer<T>::MappedData(uint32_t index)
{
	assert(m_buffer.Get());
	if (index > m_elementCount)
		index = 0;
	return m_mappedData[index];
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(const T& data, uint32_t index)
{
	assert(m_isCopyable);

	if (index >= m_elementCount) 
		return false;

	m_mappedData[index] = data;
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(const T* pArrayData, uint32_t elementCount)
{
	assert(m_isCopyable);

	if (elementCount == 0 || elementCount != m_elementCount)
		return false;

	for (uint32_t i = 0; i < m_elementCount; ++i)
		m_mappedData[i] = pArrayData[i];
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(const std::vector<T>& arrayData)
{
	assert(m_isCopyable);

	size_t arraySize = arrayData.size();
	if (arraySize == 0 || arraySize != m_elementCount)
		return false;

	std::copy(arrayData.begin(), arrayData.end(), m_mappedData);
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::EnableCopy()
{
	if (m_isCopyable) return false;
	m_isCopyable = false;
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::DisableCopy()
{
	if (!m_isCopyable) return false;
	m_isCopyable = true;
	m_buffer->Unmap(0, nullptr);
	return false;
}


