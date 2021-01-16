#include "D12Helper.h"

#include <vector>

#include <d3dcompiler.h>
#include <DirectXTex.h>

#include "StringHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

namespace
{
    class HrException : public std::runtime_error
    {
    public:
        HrException(HRESULT hr);
        HRESULT Error() const;
    private:
        HRESULT m_hr;
        friend std::string D12Helper::HrToString(HRESULT hr);
    };

    HrException::HrException(HRESULT hr) :std::runtime_error(D12Helper::HrToString(hr)), m_hr(hr)
    {
    }

    HRESULT HrException::Error() const
    {
        return m_hr;
    }
}

ComPtr<ID3D12Resource> D12Helper::CreateBuffer(ID3D12Device* pDevice ,size_t sizeInBytes, D3D12_HEAP_TYPE heapType,
    D3D12_RESOURCE_STATES resourceState)
{
    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
    auto rsDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeInBytes);
    ComPtr<ID3D12Resource> buffer = nullptr;
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        resourceState,
        nullptr,
        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf())));
    return buffer;
}

ComPtr<ID3D12Resource> D12Helper::CreateTexture2D(ID3D12Device* pdevice, UINT64 width, UINT height, 
     DXGI_FORMAT texFormat, D3D12_RESOURCE_FLAGS flag, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES state,
    const D3D12_CLEAR_VALUE* clearValue)
{
    auto rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, width, height);
    rsDesc.Flags = flag;

    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);

    ComPtr<ID3D12Resource> buffer = nullptr;
    ThrowIfFailed(pdevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        state,
        clearValue,
        IID_PPV_ARGS(buffer.GetAddressOf())
    ));
    return buffer;
}

bool D12Helper::CreateDescriptorHeap(ID3D12Device* pDevice, ComPtr<ID3D12DescriptorHeap>& pDescriptorHeap,
    UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, bool isShaderVisible, UINT nodeMask)
{
    D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
    descHeap.NodeMask = 0;
    descHeap.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descHeap.NumDescriptors = numDesciprtor;
    descHeap.Type = heapType;
    ThrowIfFailed(pDevice->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(pDescriptorHeap.ReleaseAndGetAddressOf())));
    return false;
}

void D12Helper::OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob)
{
    if (errBlob != nullptr)
    {
        OutputDebugStringA((char*)errBlob->GetBufferPointer());
    }
}

size_t D12Helper::AlignedValue(size_t value, size_t align)
{
    return (value + align - 1) & ~(align - 1);
    return value + (align - (value % align)) % align;
}

size_t D12Helper::AlignedConstantBufferMemory(size_t byteSize)
{
    return AlignedValue(byteSize, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
}

void D12Helper::ThrowIfFailed(HRESULT hr)
{
#if defined (_DEBUG) || defined (DEBUG)
    assert(SUCCEEDED(hr));
#endif
    if (FAILED(hr))
        throw HrException(hr);
}

ComPtr<ID3DBlob> D12Helper::CompileShaderFromFile(const wchar_t* filePath, const char* entryName, const char* targetVersion, const D3D_SHADER_MACRO* defines)
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


ComPtr<ID3D12Resource> D12Helper::CreateTextureFromFilePath(ID3D12Device* pDevice, const std::wstring& path)
/*-----------------LOAD TEXTURE-----------------*/
// Load texture from file path using varaible and method DirectXTex library
{
    HRESULT result = S_OK;
    
    TexMetadata metadata;
    ScratchImage scratch;

    auto fileExtension = StringHelper::GetFileExtensionW(path);
    // Direct Draw Surface
    if (fileExtension == L"dds") 
        result = LoadFromDDSFile(path.c_str(), DDS_FLAGS_FORCE_RGB, &metadata, scratch);
    // High Dynamic Range
    else if (fileExtension == L"hdr") 
        result = LoadFromHDRFile(path.c_str(), &metadata, scratch);
    // Truevision Graphics Adapter
    else if (fileExtension == L"tga") 
        result = LoadFromTGAFile(path.c_str(), &metadata, scratch);
    // WIC ( Windows Imaging Component )
    else 
        result = LoadFromWICFile(path.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratch);
    if (FAILED(result)) return nullptr;
    
    /*-----------------CREATE BUFFER-----------------*/
    // Use loaded texture to create buffer
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

    CD3DX12_RESOURCE_DESC rsDesc = {};
    switch (metadata.dimension)
    {
    case TEX_DIMENSION_TEXTURE1D:
        rsDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, metadata.width, metadata.arraySize, metadata.mipLevels);
        break;
    case TEX_DIMENSION_TEXTURE2D:
        rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
        break;
    case TEX_DIMENSION_TEXTURE3D:
        rsDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, metadata.width, metadata.height, metadata.depth, metadata.mipLevels);
        break;
    default:
        return nullptr;
    }

    ComPtr<ID3D12Resource> buffer = nullptr;
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.GetAddressOf())
    ));

    auto image = scratch.GetImages();
    const size_t num_images = scratch.GetImageCount();
    for (size_t i = 0; i < num_images; ++i)
    {
        ThrowIfFailed(buffer->WriteToSubresource(
            i,
            nullptr,
            image[i].pixels,
            image[i].rowPitch,
            image[i].slicePitch));
    }

    return buffer;
}

Microsoft::WRL::ComPtr<ID3D12Resource> D12Helper::CreateTextureFromFilePath(ID3D12Device* pDevice, 
    ID3D12GraphicsCommandList* pCmdList, ComPtr<ID3D12Resource>& uploadResource, const std::wstring& path)
{
    HRESULT result = S_OK;

    TexMetadata metadata;
    ScratchImage scratch;

    auto fileExtension = StringHelper::GetFileExtensionW(path);
    // Direct Draw Surface
    if (fileExtension == L"dds")
        result = LoadFromDDSFile(path.c_str(), DDS_FLAGS_FORCE_RGB, &metadata, scratch);
    // High Dynamic Range
    else if (fileExtension == L"hdr")
        result = LoadFromHDRFile(path.c_str(), &metadata, scratch);
    // Truevision Graphics Adapter
    else if (fileExtension == L"tga")
        result = LoadFromTGAFile(path.c_str(), &metadata, scratch);
    // WIC ( Windows Imaging Component )
    else
        result = LoadFromWICFile(path.c_str(), WIC_FLAGS_FORCE_RGB, &metadata, scratch);
    if (FAILED(result)) return nullptr;

    /*-----------------CREATE BUFFER-----------------*/
    CD3DX12_RESOURCE_DESC rsDesc = {};
    switch (metadata.dimension)
    {
    case TEX_DIMENSION_TEXTURE1D:
        rsDesc = CD3DX12_RESOURCE_DESC::Tex1D(metadata.format, metadata.width, metadata.arraySize, metadata.mipLevels);
        break;
    case TEX_DIMENSION_TEXTURE2D:
        rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(metadata.format, metadata.width, metadata.height, metadata.arraySize, metadata.mipLevels);
        break;
    case TEX_DIMENSION_TEXTURE3D:
        rsDesc = CD3DX12_RESOURCE_DESC::Tex3D(metadata.format, metadata.width, metadata.height, metadata.depth, metadata.mipLevels);
        break;
    default:
        return nullptr;
    }

    ComPtr<ID3D12Resource> texture = nullptr;
    auto heapProp = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(pDevice->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(texture.GetAddressOf())
    ));

    auto image = scratch.GetImages();
    const size_t num_images = scratch.GetImageCount();
    std::vector<D3D12_SUBRESOURCE_DATA> subreousrces;
    subreousrces.reserve(num_images);
    for (size_t i = 0; i < num_images; ++i)
    {
        subreousrces.push_back({ static_cast<void*>(image[i].pixels),
                                static_cast<LONG_PTR>(image[i].rowPitch),
                                static_cast<LONG_PTR>(image[i].slicePitch) });
    }

    UpdateDataToTextureBuffer(pDevice, pCmdList, texture, uploadResource, subreousrces.data(), subreousrces.size());

    return texture;
}

ComPtr<ID3D12Resource> D12Helper::CreateDefaultBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList,
    ComPtr<ID3D12Resource>& emptyUploadBuffer, const void* pData, size_t dataSize)
{
    ComPtr<ID3D12Resource> buffer = nullptr;
    emptyUploadBuffer.Reset();

    emptyUploadBuffer = D12Helper::CreateBuffer(pDevice, dataSize, D3D12_HEAP_TYPE_UPLOAD);
    buffer = D12Helper::CreateBuffer(pDevice, dataSize, D3D12_HEAP_TYPE_DEFAULT);
    
    D3D12_SUBRESOURCE_DATA subResource = {};
    subResource.pData = pData;
    subResource.RowPitch = dataSize;
    subResource.SlicePitch = subResource.RowPitch;

    UpdateSubresources(pCmdList, buffer.Get(), emptyUploadBuffer.Get(), 0, 0, 1, &subResource);

    return buffer;
}

bool D12Helper::UpdateDataToTextureBuffer(ID3D12Device* pDevice, ID3D12GraphicsCommandList* pCmdList,
    ComPtr<ID3D12Resource>& textureBuffer, ComPtr<ID3D12Resource>& emptyUploadBuffer, 
    const D3D12_SUBRESOURCE_DATA* pSubresources, uint32_t numResources)
{
    emptyUploadBuffer.Reset();

    auto uploadBufferSize = GetRequiredIntermediateSize(textureBuffer.Get(), 0, numResources);
    emptyUploadBuffer = D12Helper::CreateBuffer(pDevice, uploadBufferSize, D3D12_HEAP_TYPE_UPLOAD);

    // 中でcmdList->CopyTextureRegionが走っているため
    // コマンドキューうを実行して待ちをしなければならない
    UpdateSubresources(pCmdList, textureBuffer.Get(), emptyUploadBuffer.Get(), 0, 0, numResources, pSubresources);

    return true;
}

void D12Helper::ChangeResourceState(ID3D12GraphicsCommandList* pCmdList, ID3D12Resource* pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter)
{
    auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(pResource, stateBefore, stateAfter);
    pCmdList->ResourceBarrier(1, &resourceBarrier);
}

std::string D12Helper::HrToString(HRESULT hr)
{
    char s_str[64] = {};
    sprintf_s(s_str, "HRESULT of 0x%08X", static_cast<UINT>(hr));
    return std::string(s_str);
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

const D3D12_SUBRESOURCE_DATA* UpdateTextureBuffers::GetSubresource(const std::string& bufferName)
{
    // Check if there is any buffer
    assert(m_updateBuffers.size());
    // Check if bufferName is in this list
    assert(m_updateBuffers.count(bufferName));

    return &m_updateBuffers[bufferName].second;
}

ComPtr<ID3D12Resource>& UpdateTextureBuffers::GetBuffer(const std::string& bufferName)
{
    // Check if there is any buffer
    assert(m_updateBuffers.size());
    // Check if bufferName is in this list
    assert(m_updateBuffers.count(bufferName));

    return m_updateBuffers[bufferName].first;
}
