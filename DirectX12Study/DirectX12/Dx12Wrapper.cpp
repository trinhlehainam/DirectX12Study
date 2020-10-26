#include "Dx12Wrapper.h"
#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include <cassert>
#include <algorithm>
#include <string>
#include <DirectXmath.h>
#include <d3dcompiler.h>
#include <DirectXTex.h>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")

using namespace DirectX;
struct Vertex
{
    XMFLOAT3 pos;
    XMFLOAT2 uv;
};
std::vector<Vertex> vertices_;

unsigned int AlignedValue(unsigned int value, unsigned int align)
{
    return value + (align - (value % align)) % align;
}

void CreateVertices()
{                       
                           // Position               // UV
    vertices_.push_back({ { 0.0f, 100.0f, 0.0f },   { 0.0f, 1.0f } });          // bottom left  
    vertices_.push_back({ { 0.0f, 0.0f, 0.0f },    { 0.0f, 0.0f } });           // top left
    vertices_.push_back({ { 100.0f, 100.0f, 0.0f },    { 1.0f, 1.0f } });       // bottom right
    vertices_.push_back({ { 100.0f, 0.0f, 0.0f },     { 1.0f, 0.0f } });        // top right
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
    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(vertices_[0])*vertices_.size();  // 4*3*3 = 36 bytes
    resDesc.Height = 1;
    resDesc.SampleDesc.Count = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
    
    auto result = dev_->CreateCommittedResource(&heapProp,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
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

bool Dx12Wrapper::CreateTexure()
{
    HRESULT result = S_OK;
    CoInitializeEx(0, COINIT_MULTITHREADED);

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

    auto image = scratch.GetImage(0,0,0);
    result = textureBuffer_->WriteToSubresource(0,
        nullptr,
        image->pixels,
        image->rowPitch,
        image->slicePitch);
    assert(SUCCEEDED(result));

    // Create SRV Desciptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = 1;
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    desc.NodeMask = 0;
    result = dev_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&resourceViewHeap_));
    assert(SUCCEEDED(result));

    // Create SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = metadata.format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(0,1,2,3); // ※

    dev_->CreateShaderResourceView(
        textureBuffer_,
        &srvDesc,
        resourceViewHeap_->GetCPUDescriptorHandleForHeapStart()
    );

    auto cbDesc = constantBuffer_->GetDesc();
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = constantBuffer_->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = cbDesc.Width;
    auto heapPos = resourceViewHeap_->GetCPUDescriptorHandleForHeapStart();
    heapPos.ptr += dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    dev_->CreateConstantBufferView(&cbvDesc, heapPos);

    // Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rtSigDesc.NumParameters = 1;
    D3D12_ROOT_PARAMETER rp[1] = {};
    rp[0].DescriptorTable.NumDescriptorRanges = 1;
    D3D12_DESCRIPTOR_RANGE range[2] = {};
    range[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;       // t
    range[0].BaseShaderRegister = 0;                            // 0つまり(t0) を表す
    range[0].NumDescriptors = 1;                                // t0->t0まで
    range[0].OffsetInDescriptorsFromTableStart = 0;
    range[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;       // t
    range[1].BaseShaderRegister = 0;                            // 0つまり(t0) を表す
    range[1].NumDescriptors = 1;                                // t0->t0まで
    range[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    rp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    rp[0].DescriptorTable.pDescriptorRanges = range;
    rp[0].DescriptorTable.NumDescriptorRanges = _countof(range);
    rtSigDesc.pParameters = rp;
    rtSigDesc.NumParameters = _countof(rp);

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

    return true;
}

bool Dx12Wrapper::CreateConstantBuffer()
{
    HRESULT result = S_OK;
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CreationNodeMask = 0;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;

    D3D12_RESOURCE_DESC rsDesc = {};
    rsDesc.Format = DXGI_FORMAT_UNKNOWN;
    rsDesc.Width = AlignedValue(sizeof(XMMATRIX),D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);
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
        IID_PPV_ARGS(&constantBuffer_));
    assert(SUCCEEDED(result));

    XMMATRIX tempMat = XMMatrixIdentity();
    XMMATRIX* mat;
    constantBuffer_->Map(0, nullptr, (void**)&mat);
    tempMat.r[0].m128_f32[0] = 1.0f / 640.f;
    tempMat.r[1].m128_f32[1] = -1.0f / 360.0f;
    tempMat.r[0].m128_f32[3] = -1.0f;
    tempMat.r[1].m128_f32[3] = 1.0f;
    *mat = tempMat;
    constantBuffer_->Unmap(0, nullptr);

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
    gpsDesc.DepthStencilState.DepthEnable = false;
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
    //ID3DBlob* rootSigBlob = nullptr;
    //D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    //rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    //rootSigDesc.NumParameters = 0;
    //rootSigDesc.NumStaticSamplers = 0;
    //result = D3D12SerializeRootSignature(&rootSigDesc,
    //    D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
    //    &rootSigBlob,
    //    &errBlob);
    //OutputFromErrorBlob(errBlob);
    //assert(SUCCEEDED(result));
    //result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig_));
    //assert(SUCCEEDED(result));
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
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_1_0_CORE;
    HRESULT result = S_OK;

#if _DEBUG
    ID3D12Debug* debug = nullptr;
    D3D12GetDebugInterface(IID_PPV_ARGS(&debug));
    debug->EnableDebugLayer();
    debug->Release();

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&dxgi_));
#else
    result = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgi_));
#endif
    assert(SUCCEEDED(result));

    for (auto& fLevel : featureLevels)
    {
        result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(&dev_));
        if (SUCCEEDED(result)) {
            featureLevel = fLevel;
            break;
        }
    }
    if (featureLevel == D3D_FEATURE_LEVEL_1_0_CORE)
    {
        OutputDebugString(L"Feature level not found");
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC cmdQdesc = {};
    cmdQdesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQdesc.NodeMask = 0;
    cmdQdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQdesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    result = dev_->CreateCommandQueue(&cmdQdesc, IID_PPV_ARGS(&cmdQue_));
    assert(SUCCEEDED(result));

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
    CreateVertexBuffer();
    CreateConstantBuffer();
    CreateTexure();
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

bool Dx12Wrapper::Update()
{
    cmdAlloc_->Reset();
    cmdList_->Reset(cmdAlloc_,pipeline_);
    //cmdList_->Reset(cmdAlloc_, nullptr);
    //cmdList_->SetPipelineState(pipeline_);

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
    const auto rtvIncreSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    rtvHeap.ptr += static_cast<ULONG_PTR>(bbIdx) * rtvIncreSize;
    float bgColor[4] = { 0.0f,0.0f,0.0f,1.0f };
    cmdList_->OMSetRenderTargets(1, &rtvHeap, false, nullptr);
    cmdList_->ClearRenderTargetView(rtvHeap, bgColor, 0 ,nullptr);

    cmdList_->SetGraphicsRootSignature(rootSig_);
    cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    
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

    ID3D12DescriptorHeap* descHeaps[] = { resourceViewHeap_ };
    cmdList_->SetDescriptorHeaps(1, descHeaps);
    cmdList_->SetGraphicsRootDescriptorTable(0, resourceViewHeap_->GetGPUDescriptorHandleForHeapStart());

    // Draw Triangle
    cmdList_->IASetVertexBuffers(0, 1, &vbView_);
    cmdList_->DrawInstanced(vertices_.size(), 1, 0, 0);

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
