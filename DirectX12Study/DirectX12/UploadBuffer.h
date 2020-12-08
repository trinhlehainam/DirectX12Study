#pragma once
#include <vector>
#include "Dx12Helper.h"

using Microsoft::WRL::ComPtr;

// Manage resource type upload and its data
template<typename T>
class UploadBuffer
{
public:
	UploadBuffer() = default;
	UploadBuffer(ComPtr<ID3D12Device>& device, uint16_t elementCount = 1, bool isConstantBuffer = false);
	~UploadBuffer();

	bool Create(ComPtr<ID3D12Device>& device, uint16_t elementCount = 1, bool isConstantBuffer = false);
	ID3D12Resource* GetBuffer();
	D3D12_GPU_VIRTUAL_ADDRESS GetGPUVirtualAddress();

	size_t Size() const;
	size_t ElementSize() const;

	// If buffer has MULTIPLE elements of data, get data of input index
	// If buffer has only one element , index is automatically set to default (0)
	T& GetData(uint16_t index = 0);

	// If buffer has MULTIPLE elements, copy data to specific index
	// return false if index is invalid (index is larger than number of element)
	bool CopyData(const T& data, uint16_t index = 0);

	// Copy an array data to buffer
	// return false if elementCount is zero (0) 
	// or different with number of element
	bool CopyData(const T* pArrayData, size_t elementCount = 1);

	// return false if the size of array is empty 
	// or different with number of element
	bool CopyData(const std::vector<T>& arrayData);
private:
	UploadBuffer(const UploadBuffer&) = delete;
	UploadBuffer& operator = (const UploadBuffer&) = delete;

private:
	ComPtr<ID3D12Resource> m_buffer = nullptr;
	T* m_mappedData = nullptr;
	uint16_t m_elementCount = 0;
	bool m_isConstantBuffer = false;
};

template<typename T>
inline UploadBuffer<T>::UploadBuffer(ComPtr<ID3D12Device>& device, uint16_t elementCount, bool isConstantBuffer)
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
	if (m_buffer != nullptr)
		m_buffer->Unmap(0, nullptr);
	m_mappedData = nullptr;
}

template<typename T>
inline bool UploadBuffer<T>::Create(ComPtr<ID3D12Device>& device, uint16_t elementCount, bool isConstantBuffer)
{
	m_elementCount = elementCount;
	m_isConstantBuffer = isConstantBuffer;

	m_buffer = Dx12Helper::CreateBuffer(device, Size());
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));

	return false;
}

template<typename T>
inline ID3D12Resource* UploadBuffer<T>::GetBuffer()
{
	return m_buffer.Get();
}

template<typename T>
inline D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<T>::GetGPUVirtualAddress()
{
	return m_buffer.Get()->GetGPUVirtualAddress();
}

template<typename T>
inline size_t UploadBuffer<T>::Size() const
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
inline T& UploadBuffer<T>::GetData(uint16_t index)
{
	if (index > m_elementCount)
		index = 0;
	return m_mappedData[index];
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(const T& data, uint16_t index)
{
	if (index >= m_elementCount) 
		return false;

	m_mappedData[index] = data;
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(const T* pArrayData, size_t elementCount)
{
	if (elementCount == 0 || elementCount != m_elementCount)
		return false;

	for (uint16_t i = 0; i < m_elementCount; ++i)
		m_mappedData[i] = pArrayData[i];
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(const std::vector<T>& arrayData)
{
	size_t arraySize = arrayData.size();
	if (arraySize == 0 || arraySize != m_elementCount)
		return false;

	std::copy(arrayData.begin(), arrayData.end(), m_mappedData);
	return true;
}


