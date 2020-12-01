#include "Dx12Wrapper.h"
#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include "../VMDLoader/VMDMotion.h"
#include <cassert>
#include <algorithm>
#include <unordered_map>
#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <iostream>

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
    constexpr unsigned int material_descriptor_count = 5;
    //const char* model_path = "resource/PMD/model/桜ミク/mikuXS桜ミク.pmd";
    //const char* model_path = "resource/PMD/model/桜ミク/mikuXS雪ミク.pmd";
    //const char* model_path = "resource/PMD/model/霊夢/reimu_G02.pmd";
    const char* model1_path = "resource/PMD/model/初音ミク.pmd";
    const char* model3_path = "resource/PMD/model/柳生/柳生Ver1.12SW.pmd";
    const char* model2_path = "resource/PMD/model/hibiki/我那覇響v1_グラビアミズギ.pmd";

    const char* motion1_path = "resource/VMD/ヤゴコロダンス.vmd";
    const char* motion2_path = "resource/VMD/swing2.vmd";

    size_t frameNO = 0;
    float lastTick = 0;

    constexpr float animation_speed = millisecond_per_frame / second_to_millisecond;
    float timer = animation_speed;
    float angle = 0;
    float scalar = 0.1;
    constexpr float scale_speed = 1;
}

unsigned int AlignedValue(unsigned int value, unsigned int align)
{
    return value + (align - (value % align)) % align;
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

void Dx12Wrapper::CreateDefaultTexture()
{
    HRESULT result = S_OK;

    // 転送先
    whiteTexture_ = CreateTex2DBuffer(4, 4);
    blackTexture_ = CreateTex2DBuffer(4, 4);
    gradTexture_ = CreateTex2DBuffer(4, 4);

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

    auto dst = CD3DX12_TEXTURE_COPY_LOCATION(texBuffer);
    // 中でcmdList->CopyTextureRegionが走っているため
    // コマンドキューうを実行して待ちをしなければならない
    cmdList_->Reset(cmdAlloc_.Get(), nullptr);
    cmdAlloc_->Reset();
    UpdateSubresources(cmdList_.Get(), texBuffer, intermediateBuffer.Get(), 0, 0, 1, &subresourcedata);
    auto resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(texBuffer,
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList_->ResourceBarrier(1,
        &resourceBarrier);

    cmdList_->Close();
    cmdQue_->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList* const*>(cmdList_.GetAddressOf()));
    GPUCPUSync();
}

void Dx12Wrapper::CreateViewForRenderTargetTexture()
{
    HRESULT result = S_OK;
    auto rsDesc = backBuffers_[0]->GetDesc();
    rsDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    float color[] = { 0.6f,0.0f,0.0f,1.0f };
    auto clearValue = CD3DX12_CLEAR_VALUE(rsDesc.Format, color);
    rtTexture_ = CreateTex2DBuffer(rsDesc.Width, rsDesc.Height, D3D12_HEAP_TYPE_DEFAULT, rsDesc.Format, rsDesc.Flags,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    CreateDescriptorHeap(passRTVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = rsDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    dev_->CreateRenderTargetView(rtTexture_.Get(), &rtvDesc, passRTVHeap_->GetCPUDescriptorHandleForHeapStart());

    // SRV
    CreateDescriptorHeap(passSRVHeap_, 3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = rsDesc.Format;
    auto heapHandle = passSRVHeap_->GetCPUDescriptorHandleForHeapStart();
    dev_->CreateShaderResourceView(rtTexture_.Get(), &srvDesc, heapHandle);
}

void Dx12Wrapper::CreateBoardPolygonVertices()
{
    XMFLOAT3 vert[] = { {-1.0f,-1.0f,0},                // bottom left
                        {1.0f,-1.0f,0},                 // bottom right
                        {-1.0f,1.0f,0},                 // top left
                        {1.0f,1.0f,0} };                // top right
    boardPolyVert_ = CreateBuffer(sizeof(vert));

    XMFLOAT3* mappedData = nullptr;
    auto result = boardPolyVert_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    std::copy(std::begin(vert), std::end(vert), mappedData);
    boardPolyVert_->Unmap(0, nullptr);

    boardVBV_.BufferLocation = boardPolyVert_->GetGPUVirtualAddress();
    boardVBV_.SizeInBytes = sizeof(vert);
    boardVBV_.StrideInBytes = sizeof(XMFLOAT3);

}

void Dx12Wrapper::CreateBoardPipeline()
{
    HRESULT result = S_OK;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    //IA(Input-Assembler...つまり頂点入力)
    //まず入力レイアウト（ちょうてんのフォーマット）

    D3D12_INPUT_ELEMENT_DESC layout[] = {
    {
    "POSITION",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    0,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0}
    };

    // Input Assembler
    psoDesc.InputLayout.NumElements = _countof(layout);
    psoDesc.InputLayout.pInputElementDescs = layout;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Vertex Shader
    ComPtr<ID3DBlob> errBlob;
    ComPtr<ID3DBlob> vsBlob;
    result = D3DCompileFromFile(
        L"shader/boardVS.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "boardVS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Pixel Shader
    ComPtr<ID3DBlob> psBlob;
    result = D3DCompileFromFile(
        L"shader/boardPS.hlsl",                              // path to shader file
        nullptr,                                        // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
        "boardPS",                                           // entry name
        "ps_5_1",                                       // shader version
        0, 0,                                            // Flag1, Flag2 (unknown)
        psBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());

    // Other set up

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
    psoDesc.pRootSignature = boardRootSig_.Get();

    result = dev_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(boardPipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));

}

void Dx12Wrapper::CreateNormalMapTexture()
{
    CreateTextureFromFilePath(L"resource/image/normalmap3.png", normalMapTex_);

    HRESULT result = S_OK;
    auto rsDesc = backBuffers_[0]->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = rsDesc.Format;
    auto heapHandle = passSRVHeap_->GetCPUDescriptorHandleForHeapStart();

    auto heapSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    heapHandle.ptr += heapSize;
    srvDesc.Format = normalMapTex_.Get()->GetDesc().Format;
    dev_->CreateShaderResourceView(normalMapTex_.Get(), &srvDesc, heapHandle);
}

void Dx12Wrapper::CreateTimeBuffer()
{
    auto strideBytes = AlignedValue(sizeof(float), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
    timeBuffer_ = CreateBuffer(strideBytes);

    timeBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&time_));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = timeBuffer_->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = strideBytes;
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(passSRVHeap_->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(2, dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    dev_->CreateConstantBufferView(&cbvDesc, heapHandle);
}

void Dx12Wrapper::CreateBoardRootSignature()
{
    HRESULT result = S_OK;
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE range[2] = {};

    range[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
    range[1] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    D3D12_ROOT_PARAMETER parameter[2] = {};
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[0], 1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[1], 1, &range[1], D3D12_SHADER_VISIBILITY_ALL);

    rtSigDesc.NumParameters = _countof(parameter);
    rtSigDesc.pParameters = parameter;

    //サンプラらの定義、サンプラとはUVが0未満とか1超えとかのときの
    //動きyや、UVをもとに色をとってくるときのルールを指定するもの
    D3D12_STATIC_SAMPLER_DESC samplerDesc[2] = {};
    //WRAPは繰り返しを表す。
    CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0],
        0,                                  // shader register location
        D3D12_FILTER_MIN_MAG_MIP_LINEAR);    // Filter    

    CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[1],
        1,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    samplerDesc[1].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;

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
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(boardRootSig_.GetAddressOf()));
    assert(SUCCEEDED(result));
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
    
    CreateDescriptorHeap(bbRTVHeap_, back_buffer_count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    backBuffers_.resize(back_buffer_count);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = swDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    auto heap = bbRTVHeap_->GetCPUDescriptorHandleForHeapStart();
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
    HRESULT result = S_OK;
    auto rtvDesc = backBuffers_[0]->GetDesc();
    const auto depth_resource_format = DXGI_FORMAT_D32_FLOAT;

    CD3DX12_CLEAR_VALUE clearValue(
        depth_resource_format                       // format
        ,1.0f                                       // depth
        ,0);                                        // stencil

    depthBuffer_ = CreateTex2DBuffer(rtvDesc.Width, rtvDesc.Height,
        D3D12_HEAP_TYPE_DEFAULT, 
        depth_resource_format,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    CreateDescriptorHeap(dsvHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = depth_resource_format;
    dev_->CreateDepthStencilView(
        depthBuffer_.Get(),
        &dsvDesc,
        dsvHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void Dx12Wrapper::CreateShadowDepthEffect()
{
    CreateShadowDepthBuffer();
    CreateShadowRootSignature();
    CreateShadowPipelineState();
}

bool Dx12Wrapper::CreateShadowDepthBuffer()
{
    HRESULT result = S_OK;
    auto rtvDesc = backBuffers_[0]->GetDesc();
    const auto depth_resource_format = DXGI_FORMAT_D32_FLOAT;

    CD3DX12_CLEAR_VALUE clearValue(
        depth_resource_format                       // format
        , 1.0f                                       // depth
        , 0);                                        // stencil

    shadowDepthBuffer_ = CreateTex2DBuffer(rtvDesc.Width, rtvDesc.Height,
        D3D12_HEAP_TYPE_DEFAULT,
        depth_resource_format,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    CreateDescriptorHeap(shadowDSVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dev_->CreateDepthStencilView(
        shadowDepthBuffer_.Get(),
        &dsvDesc,
        shadowDSVHeap_->GetCPUDescriptorHandleForHeapStart());

    CreateDescriptorHeap(shadowSRVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dev_->CreateShaderResourceView(
        shadowDepthBuffer_.Get(),
        &srvDesc, 
        shadowSRVHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void Dx12Wrapper::CreateShadowRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[2] = {};
    // transform and bone
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        2,                                      // number of descriptors
        0);                                     // base shader register

    range[1] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // range type
        1,                                      // number of descriptors
        0);

    D3D12_ROOT_PARAMETER rootParam[2] = {};
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[0], 1, &range[0],D3D12_SHADER_VISIBILITY_VERTEX);
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1], 1, &range[1],D3D12_SHADER_VISIBILITY_PIXEL);

    rtSigDesc.pParameters = rootParam;
    rtSigDesc.NumParameters = _countof(rootParam);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errBlob;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), 
        IID_PPV_ARGS(shadowRootSig_.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void Dx12Wrapper::CreateShadowPipelineState()
{
    HRESULT result = S_OK;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    //IA(Input-Assembler...つまり頂点入力)
    //まず入力レイアウト（ちょうてんのフォーマット）

    D3D12_INPUT_ELEMENT_DESC layout[] = {
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
    DXGI_FORMAT_R32G32B32_FLOAT,
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
        },
        {
        "BONENO",
        0,
        DXGI_FORMAT_R16G16_UINT,
        0,
        D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        0
        },
        {
        "WEIGHT",
        0,
        DXGI_FORMAT_R32_FLOAT,
        0,
        D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
        0
        }
    };


    // Input Assembler
    psoDesc.InputLayout.NumElements = _countof(layout);
    psoDesc.InputLayout.pInputElementDescs = layout;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // Vertex Shader
    ComPtr<ID3DBlob> errBlob;
    ComPtr<ID3DBlob> vsBlob;
    result = D3DCompileFromFile(
        L"shader/ShadowVS.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "ShadowVS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Pixel Shader
    ComPtr<ID3DBlob> psBlob;
    result = D3DCompileFromFile(
        L"shader/ShadowPS.hlsl",                              // path to shader file
        nullptr,                                        // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
        "ShadowPS",                                           // entry name
        "ps_5_1",                                       // shader version
        0, 0,                                           // Flag1, Flag2 (unknown)
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
    psoDesc.pRootSignature = shadowRootSig_.Get();

    result = dev_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(shadowPipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));

}

void Dx12Wrapper::DrawShadow()
{
    cmdList_->SetPipelineState(shadowPipeline_.Get());
    XMVECTOR camPos = { -10,10,10,1 };
    XMVECTOR targetPos = { 10,0,0,1 };
    auto screenSize = Application::Instance().GetWindowSize();

    for (auto& model : pmdModelList_)
    {
        auto mappedMatrix = model->GetMappedMatrix();
        mappedMatrix->viewproj = XMMatrixLookAtRH(camPos, targetPos, { 0,1,0,0 }) *
            XMMatrixOrthographicRH(screenSize.width, screenSize.height, 0.1f, 300.f);
    }
}

ComPtr<ID3D12Resource> Dx12Wrapper::CreateBuffer(size_t size, D3D12_HEAP_TYPE heapType)
{
    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
    auto rsDesc = CD3DX12_RESOURCE_DESC::Buffer(size);
    ComPtr<ID3D12Resource> buffer;
    auto result = dev_->CreateCommittedResource(
        &heapProp,
        D3D12_HEAP_FLAG_NONE,
        &rsDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(buffer.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(result));
    return buffer;
}

ComPtr<ID3D12Resource> Dx12Wrapper::CreateTex2DBuffer(UINT64 width, UINT height, D3D12_HEAP_TYPE heapType, DXGI_FORMAT texFormat,
    D3D12_RESOURCE_FLAGS flag, D3D12_RESOURCE_STATES state, const D3D12_CLEAR_VALUE* clearValue)
{
    auto heapProp = CD3DX12_HEAP_PROPERTIES(heapType);
    auto rsDesc = CD3DX12_RESOURCE_DESC::Tex2D(texFormat, width, height, 1, 0, 1, 0, flag);
    ComPtr<ID3D12Resource> buffer;
    auto result = dev_->CreateCommittedResource(
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

bool Dx12Wrapper::CreateDescriptorHeap(ComPtr<ID3D12DescriptorHeap>& descriptorHeap, UINT numDesciprtor, D3D12_DESCRIPTOR_HEAP_TYPE heapType, 
    bool isShaderVisible, UINT nodeMask)
{
    D3D12_DESCRIPTOR_HEAP_DESC descHeap = {};
    descHeap.NodeMask = 0;
    descHeap.Flags = isShaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    descHeap.NumDescriptors = numDesciprtor;
    descHeap.Type = heapType;
    auto result = dev_->CreateDescriptorHeap(&descHeap, IID_PPV_ARGS(descriptorHeap.ReleaseAndGetAddressOf()));
    assert(SUCCEEDED(result));
	return false;
}

void Dx12Wrapper::GPUCPUSync()
{
    cmdQue_->ExecuteCommandLists(1, (ID3D12CommandList* const*)cmdList_.GetAddressOf());
    cmdQue_->Signal(fence_.Get(), ++fenceValue_);
    while (1)
    {
        if (fence_->GetCompletedValue() == fenceValue_)
            break;
    }
}

void Dx12Wrapper::CreatePostEffectTexture()
{
    CreateBoardPolygonVertices();
    CreateViewForRenderTargetTexture();
    CreateNormalMapTexture();
    CreateTimeBuffer();
    CreateBoardRootSignature();
    CreateBoardPipeline();
}

bool Dx12Wrapper::Initialize(const HWND& hwnd)
{
    HRESULT result = S_OK;

#if defined(DEBUG) || defined(_DEBUG)
    //ComPtr<ID3D12Debug> debug;
    //D3D12GetDebugInterface(IID_PPV_ARGS(debug.ReleaseAndGetAddressOf()));
    //debug->EnableDebugLayer();

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(dxgi_.ReleaseAndGetAddressOf()));

#else
    result = CreateDXGIFactory2(0, IID_PPV_ARGS(dxgi_.GetAddressOf()));
#endif
    assert(SUCCEEDED(result));

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    // Check all adapters (Graphics cards)
    UINT i = 0;
    IDXGIAdapter* pAdapter = nullptr;
    std::vector<IDXGIAdapter*> adapterList;
    while (dxgi_->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);
        std::wstring text = L"***Graphic card: ";
        text += desc.Description;
        text += L"/n";

        std::cout << text.c_str() << std::endl;
        adapterList.push_back(pAdapter);

        ++i;
    }

    for (auto& fLevel : featureLevels)
    {
        result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf()));
        /*-------Use strongest graphics card (adapter) GTX-------*/
        //result = D3D12CreateDevice(adapterList[1], fLevel, IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf()));
        if (FAILED(result)) {
            //IDXGIAdapter4* pAdapter;
            //dxgi_->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));
            //result = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf()));
            //OutputDebugString(L"Feature level not found");
            return false;
        }
    }

    // Load model vertices
   
    CreateCommandFamily();

    dev_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence_.ReleaseAndGetAddressOf()));
    fenceValue_ = fence_->GetCompletedValue();

    result = CoInitializeEx(0, COINIT_MULTITHREADED);
    assert(SUCCEEDED(result));

    CreateSwapChain(hwnd);
    CreateRenderTargetViews();
    CreateDepthBuffer();  
    CreatePostEffectTexture();
    CreateShadowDepthEffect();
    CreateDefaultTexture();

    pmdModelList_.emplace_back(std::make_shared<PMDModel>(dev_));
    pmdModelList_[0]->GetDefaultTexture(whiteTexture_, blackTexture_, gradTexture_);
    pmdModelList_[0]->LoadPMD(model1_path);
    pmdModelList_[0]->CreateModel();
    pmdModelList_[0]->LoadMotion(motion1_path);

    pmdModelList_.emplace_back(std::make_shared<PMDModel>(dev_));
    pmdModelList_[1]->GetDefaultTexture(whiteTexture_, blackTexture_, gradTexture_);
    pmdModelList_[1]->LoadPMD(model2_path);
    pmdModelList_[1]->CreateModel();
    pmdModelList_[1]->LoadMotion(motion2_path);
    pmdModelList_[1]->Transform(XMMatrixTranslation(10, 0, -10));

    pmdModelList_.emplace_back(std::make_shared<PMDModel>(dev_));
    pmdModelList_[2]->GetDefaultTexture(whiteTexture_, blackTexture_, gradTexture_);
    pmdModelList_[2]->LoadPMD(model3_path);
    pmdModelList_[2]->CreateModel();
    pmdModelList_[2]->LoadMotion(motion2_path);
    pmdModelList_[2]->Transform(XMMatrixTranslation(-15, 0, 5));
    
    return true;
}

bool Dx12Wrapper::Update()
{
    float deltaTime = GetTickCount64() / second_to_millisecond - lastTick;
    lastTick = GetTickCount64() / second_to_millisecond;

    if (timer <= 0.0f)
    {
        frameNO++;
        timer = animation_speed;
        scalar += 0.05;
        angle = 0.01f;
    }
    timer -= deltaTime;

    scalar = scalar > 5 ? 0.1 : scalar;
    auto test = *time_;
    *time_ = scalar;

    

    // Clear commands and open
    cmdAlloc_->Reset();
    cmdList_->Reset(cmdAlloc_.Get(), nullptr);

    //command list

    /*--------Set RESOURCE STATE before process----------*/
    auto bbIdx = swapchain_->GetCurrentBackBufferIndex();
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers_[bbIdx].Get(),                  // resource buffer
            D3D12_RESOURCE_STATE_PRESENT,               // state before
            D3D12_RESOURCE_STATE_RENDER_TARGET);        // state after
    cmdList_->ResourceBarrier(1, &barrier);

    /*-----------------------------------------------------*/

    auto postEffectHeap = passRTVHeap_->GetCPUDescriptorHandleForHeapStart();
    CD3DX12_CPU_DESCRIPTOR_HANDLE bbRTVHeap(bbRTVHeap_->GetCPUDescriptorHandleForHeapStart());
    auto dsvHeap = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    const auto rtvIncreSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    bbRTVHeap.Offset(bbIdx, rtvIncreSize);
    float peDefaultColor[4] = { 1.0f,0.0f,0.0f,0.0f };
    cmdList_->OMSetRenderTargets(1, &postEffectHeap, false, &dsvHeap);
    cmdList_->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList_->ClearRenderTargetView(postEffectHeap, peDefaultColor, 0, nullptr);

    // ビューポートと、シザーの設定
    auto wsize = Application::Instance().GetWindowSize();
    CD3DX12_VIEWPORT vp(backBuffers_[bbIdx].Get());
    cmdList_->RSSetViewports(1, &vp);

    CD3DX12_RECT rc(0, 0, wsize.width, wsize.height);
    cmdList_->RSSetScissorRects(1, &rc);

    DrawShadow();

    for (const auto& model : pmdModelList_)
    {
        model->Render(cmdList_, frameNO);
    }
    
    
    /*-------------------------------------------*/

    // Set resource state of postEffectTexture from RTV -> SRV
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtTexture_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList_->ResourceBarrier(1, &barrier);

    // 板ポリ
    float bbDefaultColor[] = { 0.0f,0.0f,0.0f,0.f };
    cmdList_->OMSetRenderTargets(1, &bbRTVHeap, false, nullptr);
    cmdList_->ClearRenderTargetView(bbRTVHeap, bbDefaultColor, 0, nullptr);
    cmdList_->SetPipelineState(boardPipeline_.Get());
    cmdList_->SetGraphicsRootSignature(boardRootSig_.Get());
    cmdList_->SetDescriptorHeaps(1, reinterpret_cast<ID3D12DescriptorHeap* const*>(passSRVHeap_.GetAddressOf()));

    CD3DX12_GPU_DESCRIPTOR_HANDLE shaderHeap(passSRVHeap_->GetGPUDescriptorHandleForHeapStart());
    cmdList_->SetGraphicsRootDescriptorTable(0, shaderHeap);
    // Set TimeBuffer For Shader
    shaderHeap.Offset(2, dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    cmdList_->SetGraphicsRootDescriptorTable(1, shaderHeap);

    cmdList_->IASetVertexBuffers(0, 1, &boardVBV_);
    cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList_->DrawInstanced(4, 1, 0, 0);

    /*--------Set RESOURCE STATE after finishing process--------*/
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers_[bbIdx].Get(),                  // resource buffer
            D3D12_RESOURCE_STATE_RENDER_TARGET,         // state before
            D3D12_RESOURCE_STATE_PRESENT);              // state after
    cmdList_->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtTexture_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList_->ResourceBarrier(1, &barrier);
    /*-----------------------------------------------------------*/

    cmdList_->Close();
    GPUCPUSync();

    // screen flip
    swapchain_->Present(0, 0);

    return true;
}

void Dx12Wrapper::Terminate()
{
    //boneBuffer_->Unmap(0, nullptr);
    timeBuffer_->Unmap(0, nullptr);
}
