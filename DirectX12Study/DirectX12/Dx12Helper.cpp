#include "Dx12Helper.h"
#include <d3dcompiler.h>
#include <DirectXTex.h>

using namespace DirectX;

ComPtr<ID3D12Resource> Dx12Helper::CreateBuffer(ComPtr<ID3D12Device>& device ,size_t size, D3D12_HEAP_TYPE heapType)
{
    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
    auto rsDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
    ComPtr<ID3D12Resource> buffer = nullptr;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
    return buffer;
}

ComPtr<ID3D12Resource> Dx12Helper::CreateTex2DBuffer(ComPtr<ID3D12Device>& device, UINT64 width, UINT height, 
     DXGI_FORMAT texFormat, D3D12_RESOURCE_FLAGS flag, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state,
    const D3D12_CLEAR_VALUE* clearValue)
{
    auto rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, width, height);
    rsDesc.Flags = flag;

    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);

    ComPtr<ID3D12Resource> buffer = nullptr;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        state,
        clearValue,
        IID_PPV_ARGS(buffer.GetAddressOf())
    ));
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
    ThrowIfFailed(device->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf())));
    return false;
}

void Dx12Helper::OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob)
{
    if (errBlob != nullptr)
    {
        OutputDebugStringA((char*)errBlob->GetBufferPointer());
    }
}

size_t Dx12Helper::AlignedValue(size_t value, size_t align)
{
    return (value + align - 1) & ~(align - 1);
    return value + (align - (value % align)) % align;
}

size_t Dx12Helper::AlignedConstantBufferMemory(size_t byteSize)
{
    return AlignedValue(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

void Dx12Helper::ThrowIfFailed(HRESULT hr)
{
#if defined (_DEBUG) || defined (DEBUG)
    assert(SUCCEEDED(hr));
#endif
    if (FAILED(hr))
        throw Dx12Helper::HrException(hr);
}

ComPtr<ID3DBlob> Dx12Helper::CompileShader(const wchar_t* filePath, const char* entryName, const char* targetVersion, D3D_SHADER_MACRO* defines)
{
    UINT compileFlag1 = 0;
#if defined (_DEBUG) || defined(DEBUG)
    compileFlag1 = D3DCOMPILE_DEBUG || D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ComPtr<ID3DBlob> byteCode = nullptr;
    ComPtr<ID3DBlob> errorMsg = nullptr;
    ThrowIfFailed(D3DCompileFromFile(
        filePath,
        defines,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryName,
        targetVersion,
        compileFlag1, 0,
        byteCode.GetAddressOf(),
        errorMsg.GetAddressOf()));
    OutputFromErrorBlob(errorMsg);
    return byteCode;
}

ComPtr<ID3D12Resource> Dx12Helper::CreateTextureFromFilePath(ComPtr<ID3D12Device>& device, const std::wstring& path)
{
    HRESULT result = S_OK;
    
    /*-----------------LOAD TEXTURE-----------------*/
    // Load texture from file path using varaible and method DirectXTex library
    TexMetadata metadata;
    ScratchImage scratch;
    result = LoadFromWICFile(
        path.c_str(),
        WIC_FLAGS_IGNORE_SRGB,
        &metadata,
        scratch);
    if (FAILED(result)) return nullptr;
    
    /*-----------------CREATE BUFFER-----------------*/
    // Use loaded texture to create buffer
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = metadata.format;
    rsDesc.Width = metadata.width;
    rsDesc.Height = metadata.height;
    rsDesc.DepthOrArraySize = metadata.arraySize;
    rsDesc.Dimension = static_cast<D3D12_RESOURCE_DIMENSION>(metadata.dimension);
    rsDesc.MipLevels = metadata.mipLevels;
    rsDesc.SampleDesc.Count = 1;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ComPtr<ID3D12Resource> buffer = nullptr;
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.GetAddressOf())
    ));

    auto image = scratch.GetImage(0, 0, 0);
    ThrowIfFailed(buffer->WriteToSubresource(0,
        nullptr,
        image->pixels,
        image->rowPitch,
        image->slicePitch));

    return buffer;
}

ComPtr<ID3D12Resource> Dx12Helper::CreateDefaultBuffer(ComPtr<ID3D12Device>& device, ID3D12GraphicsCommandList* pCmdList, 
    ComPtr<ID3D12Resource>& emptyUploadBuffer, const void* pData, size_t dataSize)
{
    ComPtr<ID3D12Resource> buffer = nullptr;
    emptyUploadBuffer.Reset();

    emptyUploadBuffer = Dx12Helper::CreateBuffer(device, dataSize);
    buffer = Dx12Helper::CreateBuffer(device, dataSize, D3D12_HEAP_TYPE_DEFAULT);
    
    D3D12_SUBRESOURCE_DATA subResource = {};
    subResource.pData = pData;
    subResource.RowPitch = dataSize;
    subResource.SlicePitch = subResource.RowPitch;

    UpdateSubresources(pCmdList, buffer.Get(), emptyUploadBuffer.Get(), 0, 0, 1, &subResource);
    ChangeResourceState(pCmdList, buffer.Get(), 
        D3D12_RESOURCE_STATE_COPY_DEST, 
        D3D12_RESOURCE_STATE_GENERIC_READ);

    return buffer;
}

bool Dx12Helper::UpdateDataToTextureBuffer(ComPtr<ID3D12Device>& device, ID3D12GraphicsCommandList* pCmdList,
    ComPtr<ID3D12Resource>& textureBuffer, ComPtr<ID3D12Resource>& emptyUploadBuffer, const D3D12_SUBRESOURCE_DATA& subResource)
{
    emptyUploadBuffer.Reset();

    auto uploadBufferSize = GetRequiredIntermediateSize(textureBuffer.Get(), 0, 1);
    emptyUploadBuffer = Dx12Helper::CreateBuffer(device, uploadBufferSize);

    // 中でcmdList->CopyTextureRegionが走っているため
    // コマンドキューうを実行して待ちをしなければならない
    UpdateSubresources(pCmdList, textureBuffer.Get(), emptyUploadBuffer.Get(), 0, 0, 1, &subResource);
    Dx12Helper::ChangeResourceState(pCmdList, textureBuffer.Get(), 
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    return true;
}

void Dx12Helper::ChangeResourceState(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
    auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource, stateBefore, stateAfter);
    pCmdList->ResourceBarrier(1, &resourceBarrier);
}

std::string Dx12Helper::HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
}

Dx12Helper::HrException::HrException(HRESULT hr):std::runtime_error(Dx12Helper::HrToString(hr)),m_hr(hr)
{
}

HRESULT Dx12Helper::HrException::Error() const
{
    return m_hr;
}

bool UpdateTextureBuffers::Add(const std::string& bufferName, const void* pData, LONG_PTR rowPitch,
    LONG_PTR slicePitch)
{
    if (m_updateBuffers.count(bufferName)) return false;

    m_updateBuffers[bufferName].first = nullptr;
    m_updateBuffers[bufferName].second = { pData, rowPitch, slicePitch };

    return true;
}

bool UpdateTextureBuffers::Clear()
{
    m_updateBuffers.clear();

    return m_updateBuffers.size();
}

const D3D12_SUBRESOURCE_DATA& UpdateTextureBuffers::GetSubresource(const std::string& bufferName)
{
    // Check if there is any buffer
    assert(m_updateBuffers.size());
    // Check if bufferName is in this list
    assert(m_updateBuffers.count(bufferName));

    return m_updateBuffers[bufferName].second;
}

ComPtr<ID3D12Resource>& UpdateTextureBuffers::GetBuffer(const std::string& bufferName)
{
    // Check if there is any buffer
    assert(m_updateBuffers.size());
    // Check if bufferName is in this list
    assert(m_updateBuffers.count(bufferName));

    return m_updateBuffers[bufferName].first;
}
