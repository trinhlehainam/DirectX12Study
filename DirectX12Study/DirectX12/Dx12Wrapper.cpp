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
#pragma comment(lib,"DxGuid.lib")

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

    std::string GetFileExtension(const std::string& path)
    {
        auto idx = path.rfind('.') + 1;
        return idx == std::string::npos ? "" : path.substr(idx, path.length() - idx);
    }

    std::vector<std::string> SplitFilePath(const std::string& path, const char splitter = ' *')
    {
        std::vector <std::string> ret;
        auto idx = path.rfind(splitter);
        if (idx == std::string::npos)
            ret.push_back(path);
        ret.push_back(path.substr(0, idx));
        ++idx;
        ret.push_back(path.substr(idx, path.length() - idx));
        return ret;
    }

    std::string GetTexturePathFromModelPath(const char* modelPath, const char* texturePath)
    {
        std::string ret = modelPath;
        auto idx = ret.rfind("/") + 1;

        return ret.substr(0, idx) + texturePath;
    }

    constexpr unsigned int back_buffer_count = 2;
    constexpr unsigned int material_desp_heap_count = 5;
    //const char* model_path = "resource/PMD/model/初音ミク.pmd";
    //const char* model_path = "resource/PMD/model/桜ミク/mikuXS桜ミク.pmd";
    //const char* model_path = "resource/PMD/model/satori/古明地さとり152Normal.pmd";
    const char* model_path = "resource/PMD/model/霊夢/reimu_G02.pmd";
    //const char* model_path = "resource/PMD/model/柳生/柳生Ver1.12SW.pmd";
    //const char* model_path = "resource/PMD/model/hibiki/我那覇響v1_グラビアミズギ.pmd";
    const char* pmd_path = "resource/PMD/";
    
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
    HRESULT result = S_OK;

    vertexBuffer_ = CreateBuffer(sizeof(vertices_[0]) * vertices_.size());

    Vertex* mappedData = nullptr;
    result = vertexBuffer_->Map(0,nullptr,reinterpret_cast<void**>(&mappedData));
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

    indicesBuffer_ = CreateBuffer(sizeof(indices_[0]) * indices_.size());

    ibView_.BufferLocation = indicesBuffer_->GetGPUVirtualAddress();
    ibView_.SizeInBytes = sizeof(indices_[0]) * indices_.size();
    ibView_.Format = DXGI_FORMAT_R16_UINT;

    auto indexType = indices_[0];
    decltype(indexType)* mappedIdxData = nullptr;
    result = indicesBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedIdxData));
    std::copy(std::begin(indices_), std::end(indices_), mappedIdxData);
    assert(SUCCEEDED(result));
    indicesBuffer_->Unmap(0, nullptr);
}

void Dx12Wrapper::OutputFromErrorBlob(ComPtr<ID3DBlob>& errBlob)
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

bool Dx12Wrapper::CreateTextureFromFilePath(const std::wstring& path, ComPtr<ID3D12Resource>& buffer)
{
    HRESULT result = S_OK;

    TexMetadata metadata;
    ScratchImage scratch;
    result = DirectX::LoadFromWICFile(
        path.c_str(),
        WIC_FLAGS_IGNORE_SRGB,
        &metadata,
        scratch);
    //return SUCCEEDED(result);
    if (FAILED(result)) return false;

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
        IID_PPV_ARGS(buffer.GetAddressOf())
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

    CreateTransformBuffer();
    LoadTextureToBuffer();
    CreateDefaultTexture();
    CreateMaterialAndTextureBuffer();
    CreateRootSignature();

    return true;
}

void Dx12Wrapper::LoadTextureToBuffer()
{
    auto& modelPaths = pmdModel_->GetModelPaths();
    auto& toonPaths = pmdModel_->GetToonPaths();

    textureBuffers_.resize(modelPaths.size());
    sphBuffers_.resize(modelPaths.size());
    spaBuffers_.resize(modelPaths.size());
    toonBuffers_.resize(toonPaths.size());

    for (int i = 0; i < modelPaths.size(); ++i)
    {
        if (!toonPaths[i].empty())
        {
            std::string toonPath;
            
            toonPath = GetTexturePathFromModelPath(model_path, toonPaths[i].c_str());
            if (!CreateTextureFromFilePath(ConvertStringToWideString(toonPath), toonBuffers_[i]))
            {
                toonPath = std::string(pmd_path) + "toon/" + toonPaths[i];
                CreateTextureFromFilePath(ConvertStringToWideString(toonPath), toonBuffers_[i]);
            }
        }
        if (!modelPaths[i].empty())
        {
            auto splittedPaths = SplitFilePath(modelPaths[i]);
            for (auto& path : splittedPaths)
            {
                auto ext = GetFileExtension(path);
                if (ext == "sph")
                {
                    auto sphPath = GetTexturePathFromModelPath(model_path, path.c_str());
                    CreateTextureFromFilePath(ConvertStringToWideString(sphPath), sphBuffers_[i]);
                }
                else if (ext == "spa")
                {
                    auto spaPath = GetTexturePathFromModelPath(model_path, path.c_str());
                    CreateTextureFromFilePath(ConvertStringToWideString(spaPath), spaBuffers_[i]);
                }
                else
                {
                    auto texPath = GetTexturePathFromModelPath(model_path, path.c_str());
                    CreateTextureFromFilePath(ConvertStringToWideString(texPath), textureBuffers_[i]);
                }
            }
        }
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

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[3] = {};

    // transform
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        1,                                      // number of descriptors
        0);                                     // base shader register

    // material
    range[1] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,    
        1,
        1);
    
    // texture
    range[2] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
        4,
        0
    );

    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[0], 1, &range[0]);

    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1],
        2,                                          // number of descriptor range
        &range[1],                                  // pointer to descritpor range
        D3D12_SHADER_VISIBILITY_PIXEL);             // shader visibility

    rtSigDesc.pParameters = rootParam;
    rtSigDesc.NumParameters = _countof(rootParam);

    //サンプラらの定義、サンプラとはUVが0未満とか1超えとかのときの
    //動きyや、UVをもとに色をとってくるときのルールを指定するもの
    D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
    //WRAPは繰り返しを表す。
    CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0],
        0,                                  // shader register location
        D3D12_FILTER_MIN_MAG_MIP_POINT);    // Filter     

    samplerDesc[1]= samplerDesc[0];
    samplerDesc[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    samplerDesc[1].ShaderRegister = 1;

    rtSigDesc.pStaticSamplers = samplerDesc;
    rtSigDesc.NumStaticSamplers = _countof(samplerDesc);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errBlob;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(rootSig_.GetAddressOf()));
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
    result = dev_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(transformDescHeap_.GetAddressOf()));
    assert(SUCCEEDED(result));

    transformBuffer_ = CreateBuffer(AlignedValue(sizeof(BasicMaterial), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

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

    // projection array
    viewproj *= XMMatrixPerspectiveFovRH(XM_PIDIV2,
        static_cast<float>(wSize.width) / static_cast<float>(wSize.height),
        0.1f,
        300.0f);
    //XMFLOAT4X4* mat;
    result = transformBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedBasicMatrix_));
    assert(SUCCEEDED(result));

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

void Dx12Wrapper::CreateDefaultTexture()
{
    HRESULT result = S_OK;

    // 転送先
    whiteTexture_ = CreateTex2DBuffer(4, 4, 1);
    blackTexture_ = CreateTex2DBuffer(4, 4, 1);
    gradTexture_ = CreateTex2DBuffer(4, 4, 1);

    struct Color
    {
        uint8_t r, g, b, a;
        Color() :r(0), g(0), b(0), a(0) {};
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) :
            r(red), g(green), b(blue), a(alpha) {};
    };

    std::vector<Color> texdata(4 * 4);
    std::fill(texdata.begin(), texdata.end(), Color(0xff, 0xff, 0xff, 0xff));

    D3D12_SUBRESOURCE_DATA subresourcedata = {};
    subresourcedata.pData = texdata.data();
    subresourcedata.RowPitch = sizeof(texdata[0]) * 4;
    subresourcedata.SlicePitch = texdata.size() * 4;

    // 転送元
    UpdateSubresourceToTextureBuffer(whiteTexture_.Get(), subresourcedata);

    std::fill(texdata.begin(), texdata.end(), Color(0x00, 0x00, 0x00, 0xff));
    UpdateSubresourceToTextureBuffer(blackTexture_.Get(), subresourcedata);
    
    texdata.resize(256);
    for (size_t i = 0; i < 256; ++i)
        std::fill_n(&texdata[i], 1, Color(255 - i, 255 - i, 255 - i, 0xff));
    
    UpdateSubresourceToTextureBuffer(gradTexture_.Get(), subresourcedata);
}

void Dx12Wrapper::UpdateSubresourceToTextureBuffer(ID3D12Resource* texBuffer, D3D12_SUBRESOURCE_DATA& subresourcedata)
{

    auto uploadBufferSize = GetRequiredIntermediateSize(texBuffer, 0, 1);
    ComPtr<ID3D12Resource> intermediateBuffer = CreateBuffer(uploadBufferSize);

    uint8_t* data = nullptr;
    intermediateBuffer->Map(0, nullptr, reinterpret_cast<void**>(&data));
    std::fill_n(data, uploadBufferSize, 0xff);
    intermediateBuffer->Unmap(0, nullptr);

    auto dst = CD3DX12_TEXTURE_COPY_LOCATION(texBuffer);
    // 中でcmdList->CopyTextureRegionが走っているため
    // コマンドキューうを実行して待ちをしなければならない
    cmdList_->Reset(cmdAlloc_.Get(), nullptr);
    cmdAlloc_->Reset();
    UpdateSubresources(cmdList_.Get(), texBuffer, intermediateBuffer.Get(), 0, 0, 1, &subresourcedata);
    cmdList_->ResourceBarrier(1,
        &CD3DX12_RESOURCE_BARRIER::Transition(texBuffer,
            D3D12_RESOURCE_STATE_COPY_DEST,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));

    cmdList_->Close();
    cmdQue_->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(cmdList_.GetAddressOf()));
    ExecuteAndWait();
}

bool Dx12Wrapper::CreateMaterialAndTextureBuffer()
{
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;

    auto& mats = pmdModel_->GetMaterials();
    auto& paths = pmdModel_->GetModelPaths();

    auto strideBytes = AlignedValue(sizeof(BasicMaterial), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    D3D12_RESOURCE_DESC rsDesc = {};
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
        IID_PPV_ARGS(materialBuffer_.GetAddressOf())
    );
    assert(SUCCEEDED(result));

    D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
    descHeap.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    descHeap.NodeMask = 0;
    descHeap.NumDescriptors = mats.size() * material_desp_heap_count;
    descHeap.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    result = dev_->CreateDescriptorHeap(
        &descHeap,
        IID_PPV_ARGS(materialDescHeap_.GetAddressOf())
    );
    assert(SUCCEEDED(result));
    
    auto gpuAddress = materialBuffer_->GetGPUVirtualAddress();
    auto heapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapAddress(materialDescHeap_->GetCPUDescriptorHandleForHeapStart());
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.SizeInBytes = strideBytes;
    cbvDesc.BufferLocation = gpuAddress;
    
    // Need to re-watch mapping data
    uint8_t* mappedMaterial = nullptr;
    result = materialBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&mappedMaterial));
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
        heapAddress.Offset(1, heapSize);
        
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = 1;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.PlaneSlice = 0;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0, 1, 2, 3); // ※

        // Create SRV for main texture (image)
        srvDesc.Format = textureBuffers_[i] ? textureBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
        dev_->CreateShaderResourceView(
            !textureBuffers_[i].Get() ? whiteTexture_.Get() : textureBuffers_[i].Get(),
            &srvDesc,
            heapAddress
        );
        heapAddress.Offset(1, heapSize);

        // Create SRV for sphere mapping texture (sph)
        srvDesc.Format = sphBuffers_[i] ? sphBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
        dev_->CreateShaderResourceView(
            !sphBuffers_[i].Get() ? whiteTexture_.Get() : sphBuffers_[i].Get(),
            &srvDesc,
            heapAddress
        );
        heapAddress.ptr += heapSize;

        // Create SRV for sphere mapping texture (spa)
        srvDesc.Format = spaBuffers_[i] ? spaBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
        dev_->CreateShaderResourceView(
            !spaBuffers_[i].Get() ? blackTexture_.Get() : spaBuffers_[i].Get(),
            &srvDesc,
            heapAddress
        );
        heapAddress.Offset(1, heapSize);

        // Create SRV for toon map
        srvDesc.Format = toonBuffers_[i] ? toonBuffers_[i]->GetDesc().Format : DXGI_FORMAT_R8G8B8A8_UNORM;
        dev_->CreateShaderResourceView(
            !toonBuffers_[i].Get() ? gradTexture_.Get() : toonBuffers_[i].Get(),
            &srvDesc,
            heapAddress
        );
        heapAddress.Offset(1, heapSize);
    }
    materialBuffer_->Unmap(0, nullptr);

    return true;
}

bool Dx12Wrapper::CreatePipelineStateObject()
{
    HRESULT result = S_OK;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
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

    psoDesc.InputLayout.NumElements = _countof(layout);
    psoDesc.InputLayout.pInputElementDescs = layout;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Vertex Shader
    ComPtr<ID3DBlob> errBlob;
    ComPtr<ID3DBlob> vsBlob ;
    result = D3DCompileFromFile(
        L"shader/vs.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "VS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Pixel Shader
    ComPtr<ID3DBlob> psBlob;
    result = D3DCompileFromFile(
        L"shader/ps.hlsl",                              // path to shader file
        nullptr,                                        // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
        "PS",                                           // entry name
        "ps_5_1",                                       // shader version
        0,0,                                            // Flag1, Flag2 (unknown)
        psBlob.GetAddressOf(), 
        errBlob.GetAddressOf());
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    // Other set up

    // Depth/Stencil
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.NodeMask = 0;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // Output set up
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    // Blend
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;     //※ color : ABGR
    
    // Root Signature
    psoDesc.pRootSignature = rootSig_.Get();

    result = dev_->CreateGraphicsPipelineState(&psoDesc,IID_PPV_ARGS(pipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));

    return true;
}

bool Dx12Wrapper::Initialize(const HWND& hwnd)
{
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };
    HRESULT result = S_OK;

#if defined(DEBUG) || defined(_DEBUG)
    //ComPtr<ID3D12Debug> debug;
    //D3D12GetDebugInterface(IID_PPV_ARGS(debug.ReleaseAndGetAddressOf()));
    //debug->EnableDebugLayer();

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgi_.GetAddressOf()));
#else
    result = CreateDXGIFactory2(0, IID_PPV_ARGS(dxgi_.GetAddressOf()));
#endif
    assert(SUCCEEDED(result));

    for (auto& fLevel : featureLevels)
    {
        result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(dev_.GetAddressOf()));
        if (FAILED(result)) {
            IDXGIAdapter4* pAdapter;
            dxgi_->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));
            result = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(dev_.GetAddressOf()));
            //OutputDebugString(L"Feature level not found");
            //return false;
        }
    }

    // Load model vertices
    pmdModel_ = std::make_shared<PMDModel>();
    pmdModel_->Load(model_path);
    vertices_ = pmdModel_->GetVerices();
    indices_ = pmdModel_->GetIndices();

    CreateCommandFamily();

    dev_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.GetAddressOf()));
    fenceValue_ = fence_->GetCompletedValue();

    CreateSwapChain(hwnd);
    CreateRenderTargetViews();
    CreateDepthBuffer();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateTexture();
    CreatePipelineStateObject();

    return true;
}

void Dx12Wrapper::CreateCommandFamily()
{
    const auto command_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HRESULT result = S_OK;
    D3D12_COMMAND_QUEUE_DESC cmdQdesc = {};
    cmdQdesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQdesc.NodeMask = 0;
    cmdQdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQdesc.Type = command_list_type;
    result = dev_->CreateCommandQueue(&cmdQdesc, IID_PPV_ARGS(cmdQue_.GetAddressOf()));
    assert(SUCCEEDED(result));

    result = dev_->CreateCommandAllocator(command_list_type,
        IID_PPV_ARGS(cmdAlloc_.GetAddressOf()));
    assert(SUCCEEDED(result));

    result = dev_->CreateCommandList(0, command_list_type,
        cmdAlloc_.Get(),
        nullptr,
        IID_PPV_ARGS(cmdList_.GetAddressOf()));
    assert(SUCCEEDED(result));

    cmdList_->Close();
}

void Dx12Wrapper::CreateSwapChain(const HWND& hwnd)
{
    auto& app = Application::Instance();
    auto wsize = app.GetWindowSize();
    DXGI_SWAP_CHAIN_DESC1 scDesc = {};
    scDesc.Width = wsize.width;
    scDesc.Height = wsize.height;
    scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scDesc.BufferCount = back_buffer_count;
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
    ComPtr<IDXGISwapChain1> swapchain1;
    auto result = dxgi_->CreateSwapChainForHwnd(
        cmdQue_.Get(),
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        swapchain1.ReleaseAndGetAddressOf());
    assert(SUCCEEDED(result));
    result = swapchain1.As(&swapchain_);
    assert(SUCCEEDED(result));
}

bool Dx12Wrapper::CreateRenderTargetViews()
{
    DXGI_SWAP_CHAIN_DESC1 swDesc = {};
    swapchain_->GetDesc1(&swDesc);
    
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = back_buffer_count;
    desc.NodeMask = 0;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    auto result = dev_->CreateDescriptorHeap(&desc,
        IID_PPV_ARGS(rtvHeap_.GetAddressOf()));
    assert(SUCCEEDED(result));

    backBuffers_.resize(back_buffer_count);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = swDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    auto heap = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    const auto increamentSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (int i = 0; i < back_buffer_count; ++i)
    {
        swapchain_->GetBuffer(i, IID_PPV_ARGS(backBuffers_[i].GetAddressOf()));
        dev_->CreateRenderTargetView(backBuffers_[i].Get(),
            &rtvDesc,
            heap);
        heap.ptr += increamentSize;
    }
    
    return true;
}

bool Dx12Wrapper::CreateDepthBuffer()
{
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 0;
    heapProp.VisibleNodeMask = 0;

    auto rtvDesc = backBuffers_[0]->GetDesc();

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

    CD3DX12_CLEAR_VALUE clearValue(rsDesc.Format    // format
        ,1.0f                                       // depth
        ,0);                                        // stencil

    auto result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(depthBuffer_.GetAddressOf()));
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
        depthBuffer_.Get(),
        &dsvDesc,
        dsvHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

ComPtr<ID3D12Resource> Dx12Wrapper::CreateBuffer(size_t size, D3D12_HEAP_PROPERTIES heapProp)
{
    ComPtr<ID3D12Resource> buffer;
    auto result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Buffer(size),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(result));
    return buffer;
}

ComPtr<ID3D12Resource> Dx12Wrapper::CreateTex2DBuffer(UINT64 width, UINT height, UINT16 arraySize, D3D12_HEAP_PROPERTIES heapProp, DXGI_FORMAT texFormat)
{
    ComPtr<ID3D12Resource> buffer;
    auto result = dev_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &CD3DX12_RESOURCE_DESC::Tex2D(texFormat, width, height, arraySize),
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.GetAddressOf())
    );
    assert(SUCCEEDED(result));
    return buffer;
}

bool Dx12Wrapper::Update()
{
    static float angle = 0;
    cmdAlloc_->Reset();
    cmdList_->Reset(cmdAlloc_.Get(),pipeline_.Get());

    angle += 0.01f;
    mappedBasicMatrix_->world = XMMatrixRotationY(angle);

    //command list
    auto bbIdx = swapchain_->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier =
    CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffers_[bbIdx].Get(),                  // resource buffer
        D3D12_RESOURCE_STATE_PRESENT,               // state before
        D3D12_RESOURCE_STATE_RENDER_TARGET);        // state after
    cmdList_->ResourceBarrier(1, &barrier);

    auto rtvHeap = rtvHeap_->GetCPUDescriptorHandleForHeapStart();
    auto dsvHeap = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    const auto rtvIncreSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rtvHeap.ptr += static_cast<ULONG_PTR>(bbIdx) * rtvIncreSize;
    float bgColor[4] = { 0.0f,0.0f,0.0f,1.0f };
    cmdList_->OMSetRenderTargets(1, &rtvHeap, false, &dsvHeap);
    cmdList_->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList_->ClearRenderTargetView(rtvHeap, bgColor, 0 ,nullptr);

    cmdList_->SetGraphicsRootSignature(rootSig_.Get());
    cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    // ビューポートと、シザーの設定
    auto wsize = Application::Instance().GetWindowSize();
    CD3DX12_VIEWPORT vp(backBuffers_[bbIdx].Get());
    cmdList_->RSSetViewports(1, &vp);

    CD3DX12_RECT rc(0,0,wsize.width,wsize.height);
    cmdList_->RSSetScissorRects(1, &rc);

    /*-------------Set up transform-------------*/
    cmdList_->SetDescriptorHeaps(1, (ID3D12DescriptorHeap* const*)transformDescHeap_.GetAddressOf());
    cmdList_->SetGraphicsRootDescriptorTable(0, transformDescHeap_->GetGPUDescriptorHandleForHeapStart());
    /*-------------------------------------------*/

    // Set Input Assemble
    cmdList_->IASetVertexBuffers(0, 1, &vbView_);
    cmdList_->IASetIndexBuffer(&ibView_);
    //cmdList_->DrawInstanced(vertices_.size(), 1, 0, 0);
    auto materials = pmdModel_->GetMaterials();

    /*-------------Set up material and texture-------------*/
    cmdList_->SetDescriptorHeaps(1, (ID3D12DescriptorHeap*const*)materialDescHeap_.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE materialHeapHandle(materialDescHeap_->GetGPUDescriptorHandleForHeapStart());
    auto materialHeapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    uint32_t indexOffset = 0;
    for (auto& m : materials)
    {
        cmdList_->SetGraphicsRootDescriptorTable(1, materialHeapHandle);

        cmdList_->DrawIndexedInstanced(m.indices,
            1,
            indexOffset,
            0,
            0);
        indexOffset += m.indices;
        materialHeapHandle.Offset(material_desp_heap_count , materialHeapSize);
    }
    //cmdList_->DrawIndexedInstanced(indices_.size(), 1, 0, 0, 0);
    /*-------------------------------------------*/

    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    cmdList_->ResourceBarrier(1, &barrier);

    cmdList_->Close();
    ExecuteAndWait();

    // screen flip
    swapchain_->Present(1, 0);

    return true;
}

void Dx12Wrapper::ExecuteAndWait()
{
    cmdQue_->ExecuteCommandLists(1, (ID3D12CommandList* const*)cmdList_.GetAddressOf());
    cmdQue_->Signal(fence_.Get(), ++fenceValue_);
    while (1)
    {
        if (fence_->GetCompletedValue() == fenceValue_)
            break;
    }
}

void Dx12Wrapper::Terminate()
{

}
