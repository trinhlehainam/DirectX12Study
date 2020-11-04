#include "Dx12Wrapper.h"
#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include "../PMDLoader/PMDModel.h"
#include "../common.h"
#include <cassert>
#include <algorithm>
#include <d3dcompiler.h>
#include <DirectXTex.h>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;

namespace
{
    // convert string to wide string
    std::wstring ConvertStringToWideString(const std::string& str)
    {
        std::wstring ret;
        auto wstringSize = MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            str.c_str(), str.length(), nullptr, 0);
        ret.resize(wstringSize);
        MultiByteToWideChar(
            CP_ACP,
            MB_PRECOMPOSED | MB_ERR_INVALID_CHARS,
            str.c_str(), str.length(), &ret[0], ret.length());
        return ret;
    }
}


std::vector<Vertex> vertices_;
std::vector<uint16_t> indices_;

unsigned int AlignedValue(unsigned int value, unsigned int align)
{
    return value + (align - (value % align)) % align;
}

void CreateVertices()
{
                            // Position                     // UV
    //vertices_.push_back({ { -100.0f, -100.0f, 100.0f },   { 0.0f, 1.0f } });          // bottom left  
    //vertices_.push_back({ { -100.0f, 100.0f, 100.0f },     { 0.0f, 0.0f } });           // top left
    //vertices_.push_back({ { 100.0f, 100.0f, 100.0f },     { 1.0f, 0.0f } });        // top right
    //vertices_.push_back({ { 100.0f, -100.0f, 100.0f },    { 1.0f, 1.0f } });       // bottom right
    //
    //vertices_.push_back({ { 100.0f, -100.0f, -100.0f },    { 0.0f, 1.0f } });
    //vertices_.push_back({ { 100.0f, 100.0f, -100.0f },     { 0.0f, 0.0f } });
    //vertices_.push_back({ { -100.0f, 100.0f, -100.0f },     { 1.0f, 0.0f } });
    //vertices_.push_back({ { -100.0f, -100.0f, -100.0f },   { 1.0f, 1.0f } });
   
    // 時計回り
    //indices_ = { 0,1,2,      0,2,3, 
    //             3,2,5,      3,5,4,
    //             5,6,7,      5,7,4,
    //             1,7,6,      1,0,7,
    //             1,6,5,      1,5,2,
    //             0,3,4,      0,4,7};
}

void Dx12Wrapper::CreateVertexBuffer()
{
    CreateVertices();
    // 確保する領域の仕様に関する設定
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 0;
    heapProp.VisibleNodeMask = 0;

    // 確保した領域の用に関する設定
    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = DXGI_FORMAT_UNKNOWN;
    rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rsDesc.Width = sizeof(vertices_[0])*vertices_.size();  // 4*3*3 = 36 bytes
    rsDesc.Height = 1;
    rsDesc.SampleDesc.Count = 1;
    rsDesc.DepthOrArraySize = 1;
    rsDesc.MipLevels = 1;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    auto result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer_));
    assert(SUCCEEDED(result));

    Vertex* mappedData = nullptr;
    result = vertexBuffer_->Map(0,nullptr,(void**)&mappedData);
    std::copy(std::begin(vertices_), std::end(vertices_), mappedData);
    assert(SUCCEEDED(result));
    vertexBuffer_->Unmap(0,nullptr);

    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(vertices_[0]) * vertices_.size();
    vbView_.StrideInBytes = sizeof(vertices_[0]);

}

void Dx12Wrapper::CreateIndexBuffer()
{
    HRESULT result = S_OK;
    // 確保する領域の仕様に関する設定
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 0;
    heapProp.VisibleNodeMask = 0;

    // 確保した領域の用に関する設定
    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = DXGI_FORMAT_UNKNOWN;
    rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rsDesc.Width = sizeof(indices_[0]) * indices_.size();
    rsDesc.Height = 1;
    rsDesc.SampleDesc.Count = 1;
    rsDesc.DepthOrArraySize = 1;
    rsDesc.MipLevels = 1;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indicesBuffer_));
    assert(SUCCEEDED(result));

    ibView_.BufferLocation = indicesBuffer_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(indices_[0]) * indices_.size();
    ibView_.Format = DXGI_FORMAT_R16_UINT;

    auto indexType = indices_.back();
    decltype(indexType)* mappedIdxData = nullptr;
    result = indicesBuffer_->Map(0, nullptr, (void**)&mappedIdxData);
    std::copy(std::begin(indices_), std::end(indices_), mappedIdxData);
    assert(SUCCEEDED(result));
    indicesBuffer_->Unmap(0, nullptr);
}

void Dx12Wrapper::OutputFromErrorBlob(ID3DBlob* errBlob)
{
    if (errBlob != nullptr)
    {
        std::string errStr = "";
        auto errSize = errBlob->GetBufferSize();
        errStr.resize(errSize);
        std::copy_n((char*)errBlob->GetBufferPointer(), errSize, errStr.begin());
        OutputDebugStringA(errStr.c_str());
        errBlob->Release();
    }
}

bool Dx12Wrapper::CreateShaderResource()
{
    HRESULT result = S_OK;

    TexMetadata metadata;
    ScratchImage scratch;
    result = DirectX::LoadFromWICFile(
        L"resource/image/textest.png",
        WIC_FLAGS_IGNORE_SRGB,
        &metadata,
        scratch);
    assert(SUCCEEDED(result));

    // Create buffer
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

    result = dev_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&textureBuffer_)
    );
    assert(SUCCEEDED(result));

    /*----------------Load bit map file----------------*/
    /*
    BmpLoader bmp("resource/image/sample.bmp");
    auto bsize = bmp.GetBmpSize();
    auto& rawData = bmp.GetRawData();
    std::vector<uint8_t> texData(4 * bsize.width * bsize.height);
    int texIdx = 0;

    //Read bmp file normally
    for (int i = 0; i < rawData.size(); )
    {
        texData[texIdx++] = rawData[i++];
        texData[texIdx++] = rawData[i++];
        texData[texIdx++] = rawData[i++];
        texData[texIdx++] = 0xff;
    }

    for (int y = bsize.height - 1; y >= 0; y--)
    {
        for (int x = 0; x < bsize.width; x+=4) {
            texData[texIdx++] = rawData[(x + y * bsize.width + x) * 3 + 0];
            texData[texIdx++] = rawData[(x + y * bsize.width + x) * 3 + 1];
            texData[texIdx++] = rawData[(x + y * bsize.width + x) * 3 + 2];
            texData[texIdx++] = 0xff;
        }
    }

    D3D12_BOX box;
    box.left = 0;
    box.right = bsize.width;
    box.top = 0;
    box.bottom = bsize.height;
    box.back = 1;
    box.front = 0;
    */

    auto image = scratch.GetImage(0, 0, 0);
    result = textureBuffer_->WriteToSubresource(0,
        nullptr,
        image->pixels,
        image->rowPitch,
        image->slicePitch);
    assert(SUCCEEDED(result));

    // Create SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = metadata.format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ※

    auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
    dev_->CreateShaderResourceView(
        textureBuffer_,
        &srvDesc,
        heapPos
    );
    return true;
}

bool Dx12Wrapper::CreateTexture(const std::wstring& path, ID3D12Resource*& buffer)
{
    HRESULT result = S_OK;

    TexMetadata metadata;
    ScratchImage scratch;
    result = DirectX::LoadFromWICFile(
        path.c_str(),
        WIC_FLAGS_IGNORE_SRGB,
        &metadata,
        scratch);
    assert(SUCCEEDED(result));

    // Create buffer
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

    result = dev_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&buffer)
    );
    assert(SUCCEEDED(result));

    auto image = scratch.GetImage(0, 0, 0);
    result = buffer->WriteToSubresource(0,
        nullptr,
        image->pixels,
        image->rowPitch,
        image->slicePitch);
    assert(SUCCEEDED(result));

    return true;
}

bool Dx12Wrapper::CreateTexture()
{
    HRESULT result = S_OK;
    result = CoInitializeEx(0, COINIT_MULTITHREADED);
    assert(SUCCEEDED(result));

    

    //CreateShaderResource();
    CreateTransformBuffer();

    LoadTextureToBuffer();
    CreateWhiteTexture();
    CreateMaterialAndTextureBuffer();

    CreateRootSignature();

    return true;
}

void Dx12Wrapper::LoadTextureToBuffer()
{
    auto& paths = pmdModel_->GetTexturePaths();
    texturesBuffers_.resize(paths.size());
    for (int i = 0; i < paths.size(); ++i)
    {
        if (!paths[i].empty())
            CreateTexture(ConvertStringToWideString(paths[i]), texturesBuffers_[i]);
    }
}

void Dx12Wrapper::CreateRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rtSigDesc.NumParameters = 1;
    D3D12_ROOT_PARAMETER rootParam[2] = {};
    rootParam[0].DescriptorTable.NumDescriptorRanges = 1;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[3] = {};

    // transform
    range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;       // b
    range[0].BaseShaderRegister = 0;                            // 0つまり(b0) を表す
    range[0].NumDescriptors = 1;                                // b0->b0まで
    range[0].OffsetInDescriptorsFromTableStart = 0;
    range[0].RegisterSpace = 0;

    // material
    range[1].RegisterSpace = 0;
    range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;       // b
    range[1].BaseShaderRegister = 1;                            // 1つまり(b1) を表す
    range[1].NumDescriptors = 1;                                // b1->b1まで
    range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    range[1].RegisterSpace = 0;
    
    // texture
    range[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;       // t
    range[2].BaseShaderRegister = 0;                            // 0つまり(b0) を表す
    range[2].NumDescriptors = 1;                                // b0->b0まで
    range[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    range[2].RegisterSpace = 0;

    rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rootParam[0].DescriptorTable.pDescriptorRanges = &range[0];
    rootParam[0].DescriptorTable.NumDescriptorRanges = 1;

    rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    rootParam[1].DescriptorTable.pDescriptorRanges = &range[1];
    rootParam[1].DescriptorTable.NumDescriptorRanges = 2;

    rtSigDesc.pParameters = rootParam;
    rtSigDesc.NumParameters = _countof(rootParam);

    //サンプラらの定義、サンプラとはUVが0未満とか1超えとかのときの
    //動きyや、UVをもとに色をとってくるときのルールを指定するもの
    D3D12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
    //WRAPは繰り返しを表す。
    samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    samplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
    samplerDesc[0].RegisterSpace = 0;
    samplerDesc[0].ShaderRegister = 0;
    samplerDesc[0].MaxAnisotropy = 0.0f;
    samplerDesc[0].MaxLOD = 0.0f;
    samplerDesc[0].MinLOD = 0.0f;
    samplerDesc[0].MipLODBias = 0.0f;

    rtSigDesc.pStaticSamplers = samplerDesc;
    rtSigDesc.NumStaticSamplers = _countof(samplerDesc);

    ID3DBlob* rootSigBlob = nullptr;
    ID3DBlob* errBlob = nullptr;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig_));
    assert(SUCCEEDED(result));
}

bool Dx12Wrapper::CreateTransformBuffer()
{
    HRESULT result = S_OK;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 1;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    result = dev_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&transformDescHeap_));
    assert(SUCCEEDED(result));

    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CreationNodeMask = 0;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = DXGI_FORMAT_UNKNOWN;
    rsDesc.Width = AlignedValue(sizeof(BasicMaterial),D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    rsDesc.Height = 1;
    rsDesc.MipLevels = 1;
    rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    rsDesc.DepthOrArraySize = 1;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rsDesc.SampleDesc.Count = 1;

    result = dev_->CreateCommittedResource(
    &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&transformBuffer_));
    assert(SUCCEEDED(result));

    auto wSize = Application::Instance().GetWindowSize();
    // XXMatrixIdentity
    // 1 0 0 0
    // 0 1 0 0
    // 0 0 1 0
    // 0 0 0 1
    XMMATRIX tempMat = XMMatrixIdentity();

    // world coordinate
    XMMATRIX world = XMMatrixRotationY(XM_PIDIV4);
    /*tempMat *= XMMatrixRotationY(XM_PIDIV4);*/

    // camera array (view)
    XMMATRIX viewproj = XMMatrixLookAtRH(
        { 0.0f, 10.0f, 15.0f, 1.0f },
        { 0.0f, 10.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 1.0f });
    /*tempMat *= XMMatrixLookAtRH(
        { 0.0f, 100.0f, 150.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 1.0f });*/

    // projection array
    viewproj *= XMMatrixPerspectiveFovRH(XM_PIDIV2,
        static_cast<float>(wSize.width) / static_cast<float>(wSize.height),
        0.1f,
        300.0f);
    //XMFLOAT4X4* mat;
    result = transformBuffer_->Map(0, nullptr, (void**)&mappedBasicMatrix_);
    assert(SUCCEEDED(result));
    //tempMat.r[0].m128_f32[0] = 2.0f / wSize.width;
    //tempMat.r[1].m128_f32[1] = -2.0f / wSize.height;
    //tempMat.r[3].m128_f32[0] = -1.0f;
    //tempMat.r[3].m128_f32[1] = 1.0f;
    //XMStoreFloat4x4(mat, tempMat);

    mappedBasicMatrix_->viewproj = viewproj;
    mappedBasicMatrix_->world = world;
    transformBuffer_->Unmap(0, nullptr);

    auto cbDesc = transformBuffer_->GetDesc();
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = transformBuffer_->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = cbDesc.Width;
    auto heapPos = transformDescHeap_->GetCPUDescriptorHandleForHeapStart();
    //heapPos.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    dev_->CreateConstantBufferView(&cbvDesc, heapPos);

    return true;
}

void Dx12Wrapper::CreateWhiteTexture()
{
    HRESULT result = S_OK;

    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_CUSTOM;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_L0;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_WRITE_BACK;

    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    rsDesc.Width = 4;
    rsDesc.Height = 4;
    rsDesc.DepthOrArraySize = 1;
    rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rsDesc.MipLevels = 1;
    rsDesc.SampleDesc.Count = 1;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    result = dev_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&whiteTexture_)
    );
    assert(SUCCEEDED(result));

    std::vector<uint8_t> texdata(4 * 4 * 4);
    std::fill(texdata.begin(), texdata.end(), 0xff);

    // Send texdata to whiteTexture_
    result = whiteTexture_->WriteToSubresource(0, nullptr, texdata.data(), 4 * 4, texdata.size());
    assert(SUCCEEDED(result));
}

bool Dx12Wrapper::CreateMaterialAndTextureBuffer()
{
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    D3D12_RESOURCE_DESC rsDesc = {};
    auto& mats = pmdModel_->GetMaterials();
    auto& paths = pmdModel_->GetTexturePaths();

    auto strideBytes = AlignedValue(sizeof(BasicMaterial), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    rsDesc.Width = mats.size() * strideBytes;
    rsDesc.Height = 1;
    rsDesc.DepthOrArraySize = 1;
    rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rsDesc.MipLevels = 1;
    rsDesc.SampleDesc.Count = 1;

    auto result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&materialBuffer_)
    );
    assert(SUCCEEDED(result));

    D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
    descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeap.NodeMask = 0;
    descHeap.NumDescriptors = mats.size() + paths.size();
    descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    result = dev_->CreateDescriptorHeap(
        &descHeap,
        IID_PPV_ARGS(&materialDescHeap_)
    );
    assert(SUCCEEDED(result));
    
    auto gpuAddress = materialBuffer_->GetGPUVirtualAddress();
    auto heapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    auto heapAddress = materialDescHeap_->GetCPUDescriptorHandleForHeapStart();
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.SizeInBytes = strideBytes;
    cbvDesc.BufferLocation = gpuAddress;
    
    // Need to re-watch mapping data
    uint8_t* mappedMaterial = nullptr;
    result = materialBuffer_->Map(0, nullptr, (void**)&mappedMaterial);
    assert(SUCCEEDED(result));
    for (int i = 0; i < mats.size(); ++i)
    {
        BasicMaterial* materialData = (BasicMaterial*)mappedMaterial;
        materialData->diffuse = mats[i].diffuse;
        materialData->alpha = mats[i].alpha;
        materialData->specular = mats[i].specular;
        materialData->specularity = mats[i].specularity;
        materialData->ambient = mats[i].ambient;
        cbvDesc.BufferLocation = gpuAddress;
        dev_->CreateConstantBufferView(
            &cbvDesc,
            heapAddress);
        // move memory offset
        mappedMaterial += strideBytes;
        gpuAddress += strideBytes;
        heapAddress.ptr += heapSize;
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        if (texturesBuffers_[i])
            srvDesc.Format = texturesBuffers_[i]->GetDesc().Format;
        else
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ※

        dev_->CreateShaderResourceView(
            texturesBuffers_[i] == nullptr ? whiteTexture_ : texturesBuffers_[i],
            &srvDesc,
            heapAddress
        );

        heapAddress.ptr += heapSize;
    }
    materialBuffer_->Unmap(0, nullptr);

    return true;
}

bool Dx12Wrapper::CreatePipelineState()
{
    HRESULT result = S_OK;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
    //IA(Input-Assembler...つまり頂点入力)
    //まず入力レイアウト（ちょうてんのフォーマット）
    
    D3D12_INPUT_ELEMENT_DESC layout[] = {
    // POSITION layout
    { 
    "POSITION",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    0,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0},
    {
    "NORMAL",
    0,
    DXGI_FORMAT_R32G32B32_FLOAT,                     // float2 -> [2D array] R32G32
    0,
    D3D12_APPEND_ALIGNED_ELEMENT,
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
    0
    },
    // UV layout
    {
    "TEXCOORD",                                   
    0,                                            
    DXGI_FORMAT_R32G32_FLOAT,                     // float2 -> [2D array] R32G32
    0,                                            
    D3D12_APPEND_ALIGNED_ELEMENT,                 
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   
    0
    }
    };

    gpsDesc.InputLayout.NumElements = _countof(layout);
    gpsDesc.InputLayout.pInputElementDescs = layout;
    gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Vertex Shader
    ID3DBlob* errBlob = nullptr;
    ID3DBlob* vsBlob = nullptr;
    result = D3DCompileFromFile(
        L"shader/vs.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "VS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        &vsBlob,
        &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    gpsDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    gpsDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();

    // Rasterizer
    gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    gpsDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    gpsDesc.RasterizerState.DepthClipEnable = false;
    gpsDesc.RasterizerState.MultisampleEnable = false;
    gpsDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;

    // Pixel Shader
    ID3DBlob* psBlob = nullptr;
    result = D3DCompileFromFile(
        L"shader/ps.hlsl",                              // path to shader file
        nullptr,                                        // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
        "PS",                                           // entry name
        "ps_5_1",                                       // shader version
        0,0,                                            // Flag1, Flag2 (unknown)
        &psBlob, 
        &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    gpsDesc.PS.BytecodeLength = psBlob->GetBufferSize();
    gpsDesc.PS.pShaderBytecode = psBlob->GetBufferPointer();

    // Other set up

    // Depth/Stencil
    gpsDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    gpsDesc.DepthStencilState.DepthEnable = true;
    gpsDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    gpsDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    gpsDesc.DepthStencilState.StencilEnable = false;
    gpsDesc.NodeMask = 0;
    gpsDesc.SampleDesc.Count = 1;
    gpsDesc.SampleDesc.Quality = 0;
    gpsDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // Output set up
    gpsDesc.NumRenderTargets = 1;
    gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    // Blend
    gpsDesc.BlendState.AlphaToCoverageEnable = false;
    gpsDesc.BlendState.IndependentBlendEnable = false;
    gpsDesc.BlendState.RenderTarget[0].BlendEnable = false;
    //gpsDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
    //gpsDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
    //gpsDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
    //gpsDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
    //gpsDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
    //gpsDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
    
    gpsDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;     //※ color : ABGR
    
    // Root Signature
    gpsDesc.pRootSignature = rootSig_;

    result = dev_->CreateGraphicsPipelineState(&gpsDesc,IID_PPV_ARGS(&pipeline_));
    assert(SUCCEEDED(result));

    vsBlob->Release();
    psBlob->Release();

    return true;
}

bool Dx12Wrapper::Initialize(HWND hwnd)
{
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    HRESULT result = S_OK;

#if defined(DEBUG) || defined(_DEBUG)
    //ID3D12Debug* debug = nullptr;
    //D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    //debug->EnableDebugLayer();
    //debug->Release();

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_));
#else
    result = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_));
#endif
    assert(SUCCEEDED(result));

    for (auto& fLevel : featureLevels)
    {
        result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(&dev_));
        if (FAILED(result)) {
            IDXGIAdapter4* pAdapter;
            dxgi_->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));
            D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(&dev_));
            //OutputDebugString(L"Feature level not found");
            //return false;
        }
    }

    D3D12_COMMAND_QUEUE_DESC cmdQdesc = {};
    cmdQdesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQdesc.NodeMask = 0;
    cmdQdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    result = dev_->CreateCommandQueue(&cmdQdesc, IID_PPV_ARGS(&cmdQue_));
    assert(SUCCEEDED(result));

    // Load model vertices
    pmdModel_ = std::make_shared<PMDModel>();
    pmdModel_->Load("resource/PMD/model/初音ミクVer2.pmd");
    vertices_ = pmdModel_->GetVerices();
    indices_ = pmdModel_->GetIndices();

    auto& app = Application::Instance();
    auto wsize = app.GetWindowSize();
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = wsize.width;
    scDesc.Height = wsize.height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferCount = 2;
    // ※BACK_BUFFERでもRENDER_TARGETでも動くが
    // 違いがわからないため、不具合が起きた時に動作の
    // 違いが出かどうか検証しよう
    scDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
    scDesc.Flags = 0;
    scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    scDesc.SampleDesc.Count = 1;
    scDesc.SampleDesc.Quality = 0;
    scDesc.Scaling = DXGI_SCALING_NONE;
    //scDesc.Scaling = DXGI_SCALING_STRETCH;
    scDesc.Stereo = false;
    scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    result = dxgi_->CreateSwapChainForHwnd(cmdQue_,
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        (IDXGISwapChain1**)&swapchain_);
    assert(SUCCEEDED(result));

    result = dev_->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(&cmdAlloc_));
    assert(SUCCEEDED(result));

    result = dev_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        cmdAlloc_,
        nullptr,
        IID_PPV_ARGS(&cmdList_));
    assert(SUCCEEDED(result));

    cmdList_->Close();

    dev_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
    fenceValue_ = fence_->GetCompletedValue();

    CreateRenderTargetDescriptorHeap();
    CreateDepthBuffer();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateTexture();
    CreatePipelineState();

    return true;
}

bool Dx12Wrapper::CreateRenderTargetDescriptorHeap()
{
    DXGI_SWAP_CHAIN_DESC1 swDesc = {};
    swapchain_->GetDesc1(&swDesc);
    const UINT num_descriptor_rtv = swDesc.BufferCount;
    
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 2;
    desc.NodeMask = 0;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    auto result = dev_->CreateDescriptorHeap(&desc,
        IID_PPV_ARGS(&rtvHeap_));
    assert(SUCCEEDED(result));

    bbResources_.resize(num_descriptor_rtv);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = swDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    auto heap = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    const auto increamentSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (int i = 0; i < num_descriptor_rtv; ++i)
    {
        swapchain_->GetBuffer(i, IID_PPV_ARGS(&bbResources_[i]));
        dev_->CreateRenderTargetView(bbResources_[i],
            &rtvDesc,
            heap);
        heap.ptr += increamentSize;
    }
    
    return SUCCEEDED(result);
}

bool Dx12Wrapper::CreateDepthBuffer()
{
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 0;
    heapProp.VisibleNodeMask = 0;

    auto rtvDesc = bbResources_[0]->GetDesc();

    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = DXGI_FORMAT_D32_FLOAT;
    rsDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    rsDesc.DepthOrArraySize = 1;
    rsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    rsDesc.MipLevels = 1;
    rsDesc.Width = rtvDesc.Width;
    rsDesc.Height = rtvDesc.Height;
    rsDesc.SampleDesc.Count = 1;
    rsDesc.SampleDesc.Quality = 0;
    rsDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = rsDesc.Format;
    clearValue.DepthStencil.Depth = 1.0f;

    auto result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&depthBuffer_));
    assert(SUCCEEDED(result));

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 1;
    desc.NodeMask = 0;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    result = dev_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dsvHeap_));
    assert(SUCCEEDED(result));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order NOT count of
    dsvDesc.Format = rsDesc.Format;
    dev_->CreateDepthStencilView(
        depthBuffer_,
        &dsvDesc,
        dsvHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

bool Dx12Wrapper::Update()
{
    static float angle = 0;
    cmdAlloc_->Reset();
    cmdList_->Reset(cmdAlloc_,pipeline_);
    //cmdList_->Reset(cmdAlloc_, nullptr);
    //cmdList_->SetPipelineState(pipeline_);
    angle += 0.01f;

    mappedBasicMatrix_->world = XMMatrixRotationY(angle);

    //command list
    auto bbIdx = swapchain_->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = bbResources_[bbIdx];
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    cmdList_->ResourceBarrier(1, &barrier);

    auto rtvHeap = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    auto dsvHeap = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    const auto rtvIncreSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rtvHeap.ptr += static_cast<ULONG_PTR>(bbIdx) * rtvIncreSize;
    float bgColor[4] = { 0.0f,0.0f,0.0f,1.0f };
    cmdList_->OMSetRenderTargets(1, &rtvHeap, false, &dsvHeap);
    cmdList_->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList_->ClearRenderTargetView(rtvHeap, bgColor, 0 ,nullptr);

    cmdList_->SetGraphicsRootSignature(rootSig_);
    cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // ビューポートと、シザーの設定
    auto wsize = Application::Instance().GetWindowSize();
    D3D12_VIEWPORT vp = {};
    vp.Width = wsize.width;
    vp.Height = wsize.height;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.MaxDepth = 1.0f;
    vp.MinDepth = 0.0f;
    cmdList_->RSSetViewports(1, &vp);

    D3D12_RECT rc = {};
    rc.left = 0;
    rc.top = 0;
    rc.right = wsize.width;
    rc.bottom = wsize.height;
    cmdList_->RSSetScissorRects(1, &rc);

    /*-------------Set up transform-------------*/
    cmdList_->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)&transformDescHeap_);
    cmdList_->SetGraphicsRootDescriptorTable(0, transformDescHeap_->GetGPUDescriptorHandleForHeapStart());
    /*-------------------------------------------*/

    // Draw Triangle
    cmdList_->IASetVertexBuffers(0, 1, &vbView_);
    cmdList_->IASetIndexBuffer(&ibView_);
    //cmdList_->DrawInstanced(vertices_.size(), 1, 0, 0);
    auto materials = pmdModel_->GetMaterials();

    /*-------------Set up material and texture-------------*/
    cmdList_->SetDescriptorHeaps(1, (ID3D12DescriptorHeap*const*)&materialDescHeap_);
    auto materialHeapPos = materialDescHeap_->GetGPUDescriptorHandleForHeapStart();
    auto materialHeapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    uint32_t indexOffset = 0;
    for (auto& m : materials)
    {
        cmdList_->SetGraphicsRootDescriptorTable(1, materialHeapPos);

        cmdList_->DrawIndexedInstanced(m.indices,
            1,
            indexOffset,
            0,
            0);
        indexOffset += m.indices;
        materialHeapPos.ptr += static_cast<UINT>(2 * materialHeapSize);
    }
    //cmdList_->DrawIndexedInstanced(indices_.size(), 1, 0, 0, 0);
    /*-------------------------------------------*/

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmdList_->ResourceBarrier(1, &barrier);

    cmdList_->Close();
    cmdQue_->ExecuteCommandLists(1, (ID3D12CommandList*const*)&cmdList_);
    cmdQue_->Signal(fence_, ++fenceValue_);
    while (1)
    {
        if (fence_->GetCompletedValue() == fenceValue_)
            break;
    }

    // screen flip
    swapchain_->Present(1, 0);

    return true;
}

void Dx12Wrapper::Terminate()
{
    dev_->Release();
}
