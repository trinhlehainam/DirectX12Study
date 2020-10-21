#include "Dx12Wrapper.h"
#include "../Application.h"
#include <cassert>
#include <algorithm>
#include <string>
#include <DirectXmath.h>
#include <d3dcompiler.h>

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

using namespace DirectX;

XMFLOAT3 vertices_[3];

void CreateVertices()
{
    vertices_[0] = XMFLOAT3(-1, -1, 0);
    vertices_[1] = XMFLOAT3(0, 1, 0);
    vertices_[2] = XMFLOAT3(1, -1, 0);
}

void Dx12Wrapper::CreateVertexBuffer()
{
    CreateVertices();
    // How to store resource setting
    D3D12_HEAP_PROPERTIES heapProp = {};
    heapProp.Type = D3D12_HEAP_TYPE_UPLOAD;
    heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProp.CreationNodeMask = 0;
    heapProp.VisibleNodeMask = 0;

    D3D12_RESOURCE_DESC resDesc = {};
    resDesc.Format = DXGI_FORMAT_UNKNOWN;
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Width = sizeof(vertices_);  // 4*3*3 = 36 bytes
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

    XMFLOAT3* mappedData = nullptr;
    result = vertexBuffer_->Map(0,nullptr,(void**)&mappedData);
    std::copy(std::begin(vertices_), std::end(vertices_), mappedData);
    assert(SUCCEEDED(result));
    vertexBuffer_->Unmap(0,nullptr);

    vbView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vbView_.SizeInBytes = sizeof(vertices_);
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

bool Dx12Wrapper::CreatePipelineState()
{
    HRESULT result = S_OK;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsDesc = {};
    //IA(Input-Assembler...つまり頂点入力)
    //まず入力レイアウト（ちょうてんのフォーマット）
    
    D3D12_INPUT_ELEMENT_DESC layout[] = {
    { "POSITION",
    0,
    DXGI_FORMAT_R32G32B32_FLOAT,
    0,
    D3D12_APPEND_ALIGNED_ELEMENT,
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
    0} };

    gpsDesc.InputLayout.NumElements = _countof(layout);
    gpsDesc.InputLayout.pInputElementDescs = layout;
    gpsDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    // 頂点シェーダ
    ID3DBlob* errBlob = nullptr;
    ID3DBlob* vsBlob = nullptr;
    result = D3DCompileFromFile(L"shader/vs.hlsl", nullptr, nullptr, "VS", "vs_5_1", 0, 0, &vsBlob,&errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    gpsDesc.VS.BytecodeLength = vsBlob->GetBufferSize();
    gpsDesc.VS.pShaderBytecode = vsBlob->GetBufferPointer();

    // Set up Rasterizer
    gpsDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    gpsDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    gpsDesc.RasterizerState.DepthClipEnable = false;
    gpsDesc.RasterizerState.MultisampleEnable = false;
    gpsDesc.RasterizerState.SlopeScaledDepthBias = 0.0f;

    // Set up Pixel Shader
    ID3DBlob* pxBlob = nullptr;
    result = D3DCompileFromFile(L"shader/ps.hlsl", nullptr, nullptr, "PS", "ps_5_1", 0, 0, &pxBlob, &errBlob);
    OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    gpsDesc.PS.BytecodeLength = pxBlob->GetBufferSize();
    gpsDesc.PS.pShaderBytecode = pxBlob->GetBufferPointer();

    // Other set up
    gpsDesc.DepthStencilState.DepthEnable = false;
    gpsDesc.DepthStencilState.StencilEnable = false;
    gpsDesc.NodeMask = 0;
    gpsDesc.SampleDesc.Count = 1;
    gpsDesc.SampleDesc.Quality = 0;
    gpsDesc.SampleMask = 0xffffffff;

    // Output set up
    gpsDesc.BlendState.AlphaToCoverageEnable = false;
    gpsDesc.BlendState.IndependentBlendEnable = false;
    gpsDesc.BlendState.RenderTarget[0].BlendEnable = false;
    gpsDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = 0b1111;     //※ color : ABGR
    gpsDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    gpsDesc.NumRenderTargets = 1;

    // Root Signature
    ID3DBlob* rootSigBlob = nullptr;
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSigDesc.NumParameters = 0;
    rootSigDesc.NumStaticSamplers = 0;

    result = D3D12SerializeRootSignature(&rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    assert(SUCCEEDED(result));
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), IID_PPV_ARGS(&rootSig_));
    assert(SUCCEEDED(result));
    gpsDesc.pRootSignature = rootSig_;

    result = dev_->CreateGraphicsPipelineState(&gpsDesc,IID_PPV_ARGS(&pipeline_));
    assert(SUCCEEDED(result));
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
    scDesc.Scaling = DXGI_SCALING_STRETCH;
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

    // Draw Triangle
    cmdList_->IASetVertexBuffers(0, 1, &vbView_);
    cmdList_->DrawInstanced(3, 1, 0, 0);

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
