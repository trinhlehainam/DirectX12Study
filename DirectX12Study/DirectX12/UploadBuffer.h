#pragma once
#include <vector>
#include "Dx12Helper.h"

using Microsoft::WRL::ComPtr;

// ComPtr<ID3D12Device>&	device
// uint16_t					elementCount
// bool						isConstantBuffer
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
	size_t GetBufferSize() const;

	// If buffer has MULTIPLE elements of data, get data of input index
	// But buffer has only one element , index is set to default (0)
	T& GetData(uint16_t index = 0);

	// return false if copy process is failed
	bool CopyData(uint16_t index, const T& data);
	// return false if copy process is failed
	bool CopyArrayData(const std::vector<T>& arrayData);

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

	m_buffer = Dx12Helper::CreateBuffer(device, GetBufferSize());
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
inline size_t UploadBuffer<T>::GetBufferSize() const
{
	return m_isConstantBuffer ? 
		Dx12Helper::AlignedConstantBufferMemory(sizeof(T)) * m_elementCount
		: sizeof(T) * m_elementCount;
}

template<typename T>
inline T& UploadBuffer<T>::GetData(uint16_t index)
{
	if (index > m_elementCount)
		return *m_mappedData;
	return m_mappedData[index];
}

template<typename T>
inline bool UploadBuffer<T>::CopyData(uint16_t index, const T& data)
{
	if (index >= m_elementCount) 
		return false;

	std::copy(std::begin(data), std::end(data), m_mappedData[index]);
	return true;
}

template<typename T>
inline bool UploadBuffer<T>::CopyArrayData(const std::vector<T>& arrayData)
{
	if (arrayData.size() == 0)
		return false;

	std::copy(arrayData.begin(), arrayData.end(), m_mappedData);
	return true;
}
