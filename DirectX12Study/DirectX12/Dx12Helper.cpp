#include "Dx12Helper.h"
#include <cassert>

ComPtr<ID3D12Resource> Dx12Helper::CreateBuffer(ComPtr<ID3D12Device>& device ,size_t size, D3D12_HEAP_TYPE heapType)
{
    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
    auto rsDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
    ComPtr<ID3D12Resource> buffer;
    auto result = device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(result));
    return buffer;
}

ComPtr<ID3D12Resource> Dx12Helper::CreateTex2DBuffer(ComPtr<ID3D12Device>& device, UINT64 width, UINT height, 
    D3D12_HEAP_TYPE heapType, DXGI_FORMAT texFormat, D3D12_RESOURCE_FLAGS flag, D3D12_RESOURCE_STATES state, 
    const D3D12_CLEAR_VALUE* clearValue)
{
    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
    auto rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, width, height, 1, 0, 1, 0, flag);
    ComPtr<ID3D12Resource> buffer;
    auto result = device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        state,
        clearValue,
        IID_PPV_ARGS(buffer.GetAddressOf())
    );
    assert(SUCCEEDED(result));
    return buffer;
}

bool Dx12Helper::CreateDescriptorHeap(ComPtr<ID3D12Device>& device, ComPtr<ID3D12DescriptorHeap>& descriptorHeap, 
    UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible, UINT nodeMask)
{
    D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
    descHeap.NodeMask = 0;
    descHeap.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descHeap.NumDescriptors = numDesciprtor;
    descHeap.Type = heapType;
    auto result = device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(result));
    return false;
}

void Dx12Helper::OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob)
{
    if (errBlob != nullptr)
    {
        std::string errStr = "";
        auto errSize = errBlob->GetBufferSize();
        errStr.resize(errSize);
        std::copy_n((char*)errBlob->GetBufferPointer(), errSize, errStr.begin());
        OutputDebugStringA(errStr.c_str());
    }
}

uint16_t Dx12Helper::AlignedValue(uint16_t value, uint16_t align)
{
    return (value + align - 1) & ~(align - 1);
    return value + (align - (value % align)) % align;
}

uint16_t Dx12Helper::AlignedConstantBufferMemory(uint16_t byteSize)
{
    return AlignedValue(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

void Dx12Helper::ThrowIfFailed(HRESULT hr)
{
    if (FAILED(hr))
        throw HrException(hr);
}

HrException::HrException(HRESULT hr):std::runtime_error(HrToString(hr)),m_hr(hr)
{
}

HRESULT HrException::Error() const
{
    return m_hr;
}
