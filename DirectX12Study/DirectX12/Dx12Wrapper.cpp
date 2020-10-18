#include <cassert>
#include "Dx12Wrapper.h"
#include "../Application.h"

#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")

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
    static int iR = 0;
    iR = (iR + 1) % 256;
    cmdAlloc_->Reset();
    cmdList_->Reset(cmdAlloc_,nullptr);

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
    float clrColor[4] = { iR / 255.0f,0.0f,0.0f,1.0f };
    cmdList_->OMSetRenderTargets(1, &rtvHeap, false, nullptr);
    cmdList_->ClearRenderTargetView(rtvHeap, clrColor, 0 ,nullptr);
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
