#include "UploadBuffer.h"

#ifndef UPLOADBUFFER_IMPL_H
#define UPLOADBUFFER_IMPL_H
#include <vector>
#include <cassert>
#include "../Utility/D12Helper.h"

template<typename T>
UploadBuffer<T>::UploadBuffer(ID3D12Device* device, uint32_t elementCount, bool isConstantBuffer)
{
	m_elementCount = elementCount;
	m_isConstantBuffer = isConstantBuffer;

	auto bufferSize = isConstantBuffer ? D12Helper::AlignedConstantBufferMemory(sizeof(T)) : sizeof(T);
	bufferSize *= m_elementCount;

	m_buffer = D12Helper::CreateBuffer(device, bufferSize);
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));
}

template<typename T>
inline UploadBuffer<T>::UploadBuffer(UploadBuffer&& other) noexcept
	:m_buffer(other.m_buffer),
	m_elementCount(other.m_elementCount),
	m_isCopyable(other.m_isCopyable),
	m_isConstantBuffer(other.m_isConstantBuffer),
	m_mappedData(other.m_mappedData)
{
	other.m_buffer = nullptr;
	other.m_mappedData = nullptr;
	other.m_isConstantBuffer = false;
	other.m_isCopyable = false;
	other.m_elementCount = 0;
}

template<typename T>
UploadBuffer<T>::~UploadBuffer()
{
	if (m_buffer != nullptr && m_isCopyable)
		m_buffer->Unmap(0, nullptr);
	m_mappedData = nullptr;
}

template<typename T>
bool UploadBuffer<T>::Create(ID3D12Device* pDevice, uint32_t elementCount, bool isConstantBuffer)
{
	m_elementCount = elementCount;
	m_isConstantBuffer = isConstantBuffer;

	// If this is constant buffer
	// Constant Buffer size must be 16-byte aligned
	if (m_isConstantBuffer)
	{
		size_t checkSize = m_elementCount * sizeof(T);
		assert(checkSize % 16 == 0);
	}
	//

	m_buffer = D12Helper::CreateBuffer(pDevice, SizeInBytes());
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));

	return false;
}

template<typename T>
ID3D12Resource* UploadBuffer<T>::Get()
{
	return m_buffer.Get();
}

template<typename T>
D3D12_GPU_VIRTUAL_ADDRESS UploadBuffer<T>::GetGPUVirtualAddress()
{
	assert(m_buffer.Get() != nullptr);
	return m_buffer.Get()->GetGPUVirtualAddress();
}

template<typename T>
size_t UploadBuffer<T>::SizeInBytes() const
{
	return ElementSize() * m_elementCount;
}

template<typename T>
size_t UploadBuffer<T>::ElementSize() const
{
	return m_isConstantBuffer ?
		D12Helper::AlignedConstantBufferMemory(sizeof(T))
		: sizeof(T);
}

template<typename T>
T* UploadBuffer<T>::HandleMappedData(uint32_t index)
{
	assert(m_isCopyable);
	if (!m_isCopyable) return nullptr;

	assert(m_buffer.Get());
	if (index > m_elementCount)
		index = 0;
	return &reinterpret_cast<T*>(m_mappedData)[index];
}

template<typename T>
bool UploadBuffer<T>::CopyData(const T& data, uint32_t index)
{
	assert(m_isCopyable);

	if (index >= m_elementCount)
		return false;

	reinterpret_cast<T*>(m_mappedData)[index] = data;
	return true;
}

template<typename T>
bool UploadBuffer<T>::CopyData(const T* pArrayData, uint32_t elementCount)
{
	assert(m_isCopyable);

	if (elementCount == 0 || elementCount != m_elementCount)
		return false;

	std::copy(pArrayData, pArrayData + elementCount, m_mappedData);
	return true;
}

template<typename T>
bool UploadBuffer<T>::CopyData(const std::vector<T>& arrayData)
{
	assert(m_isCopyable);

	size_t arraySize = arrayData.size();
	if (arraySize == 0 || arraySize != m_elementCount)
		return false;

	std::copy(arrayData.begin(), arrayData.end(), m_mappedData);
	return true;
}

template<typename T>
bool UploadBuffer<T>::EnableCopy()
{
	if (m_isCopyable) return false;
	m_isCopyable = false;
	m_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mappedData));
	return true;
}

template<typename T>
bool UploadBuffer<T>::DisableCopy()
{
	if (!m_isCopyable) return false;
	m_isCopyable = true;
	m_buffer->Unmap(0, nullptr);
	return false;
}

template<typename T>
inline void UploadBuffer<T>::Move(UploadBuffer& other)
{
	if(m_buffer != nullptr)
		m_buffer.Reset();
	m_buffer = other.m_buffer;
	m_elementCount = other.m_elementCount;
	m_mappedData = other.m_mappedData;
	m_isConstantBuffer = other.m_isConstantBuffer;
	m_isCopyable = other.m_isCopyable;

	other.m_mappedData = nullptr;
	other.m_buffer = nullptr;
	other.m_isCopyable = false;
	other.m_isConstantBuffer = false;
}


#endif