#include "Dx12Wrapper.h"
#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include "../VMDLoader/VMDMotion.h"
#include "Dx12Helper.h"
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
    whiteTexture_ = Dx12Helper::CreateTex2DBuffer(dev_,4, 4);
    blackTexture_ = Dx12Helper::CreateTex2DBuffer(dev_,4, 4);
    gradTexture_ = Dx12Helper::CreateTex2DBuffer(dev_,4, 4);

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
    ComPtr<ID3D12Resource> intermediateBuffer = Dx12Helper::CreateBuffer(dev_,uploadBufferSize);

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
    WaitForGPU();
}

void Dx12Wrapper::CreateViewForRenderTargetTexture()
{
    HRESULT result = S_OK;
    auto rsDesc = backBuffers_[0]->GetDesc();
    float color[] = { 0.f,0.0f,0.0f,1.0f };
    auto clearValue = CD3DX12_CLEAR_VALUE(rsDesc.Format, color);
    rtTexture_ = Dx12Helper::CreateTex2DBuffer(dev_,rsDesc.Width, rsDesc.Height, 
        D3D12_HEAP_TYPE_DEFAULT, rsDesc.Format, 
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    Dx12Helper::CreateDescriptorHeap(dev_,passRTVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = rsDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    dev_->CreateRenderTargetView(rtTexture_.Get(), &rtvDesc, passRTVHeap_->GetCPUDescriptorHandleForHeapStart());

    // SRV
    Dx12Helper::CreateDescriptorHeap(dev_,passSRVHeap_, 4, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

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
    boardPolyVert_ = Dx12Helper::CreateBuffer(dev_,sizeof(vert));

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
    Dx12Helper::OutputFromErrorBlob(errBlob);
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
    Dx12Helper::OutputFromErrorBlob(errBlob);
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

void Dx12Wrapper::RenderToPostEffectBuffer()
{
    auto wsize = Application::Instance().GetWindowSize();
    CD3DX12_VIEWPORT vp(backBuffers_[currentBackBuffer_].Get());
    cmdList_->RSSetViewports(1, &vp);

    CD3DX12_RECT rc(0, 0, wsize.width, wsize.height);
    cmdList_->RSSetScissorRects(1, &rc);

    float peDefaultColor[4] = { 1.0f,0.0f,0.0f,0.0f };
    auto postEffectHeap = passRTVHeap_->GetCPUDescriptorHandleForHeapStart();
    auto dsvHeap = dsvHeap_->GetCPUDescriptorHandleForHeapStart();
    cmdList_->OMSetRenderTargets(1, &postEffectHeap, false, &dsvHeap);
    cmdList_->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    cmdList_->ClearRenderTargetView(postEffectHeap, peDefaultColor, 0, nullptr);

    RenderPrimitive();

    for (const auto& model : pmdModelList_)
    {
        model->Render(cmdList_, frameNO);
    }

    // Set resource state of postEffectTexture from RTV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtTexture_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList_->ResourceBarrier(1, &barrier);
}

void Dx12Wrapper::RenderToBackBuffer()
{
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            backBuffers_[currentBackBuffer_].Get(),      // resource buffer
            D3D12_RESOURCE_STATE_PRESENT,               // state before
            D3D12_RESOURCE_STATE_RENDER_TARGET);        // state after
    cmdList_->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE bbRTVHeap(bbRTVHeap_->GetCPUDescriptorHandleForHeapStart());
    const auto rtvIncreSize = dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    bbRTVHeap.Offset(currentBackBuffer_, rtvIncreSize);

    float bbDefaultColor[] = { 0.0f,0.0f,0.0f,1.f };
    cmdList_->OMSetRenderTargets(1, &bbRTVHeap, false, nullptr);
    cmdList_->ClearRenderTargetView(bbRTVHeap, bbDefaultColor, 0, nullptr);

    cmdList_->SetPipelineState(boardPipeline_.Get());
    cmdList_->SetGraphicsRootSignature(boardRootSig_.Get());

    cmdList_->SetDescriptorHeaps(1, passSRVHeap_.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE shaderHeap(passSRVHeap_->GetGPUDescriptorHandleForHeapStart());
    cmdList_->SetGraphicsRootDescriptorTable(0, shaderHeap);

    cmdList_->IASetVertexBuffers(0, 1, &boardVBV_);
    cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList_->DrawInstanced(4, 1, 0, 0);

}

void Dx12Wrapper::SetBackBufferIndexForNextFrame()
{
    // Swap back buffer index to next frame use
    currentBackBuffer_ = (currentBackBuffer_ + 1) % back_buffer_count;
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
    srvDesc.Format = normalMapTex_.Get()->GetDesc().Format;

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(passSRVHeap_->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(1, dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    dev_->CreateShaderResourceView(normalMapTex_.Get(), &srvDesc, heapHandle);
}

void Dx12Wrapper::CreateShadowResourceForPostEffect()
{
    HRESULT result = S_OK;

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(passSRVHeap_->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(2, dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    dev_->CreateShaderResourceView(shadowDepthBuffer_.Get(), &srvDesc, heapHandle);
}

void Dx12Wrapper::CreateTimeBuffer()
{
    auto strideBytes = Dx12Helper::AlignedConstantBufferMemory(sizeof(float));
    timeBuffer_ = Dx12Helper::CreateBuffer(dev_,strideBytes);

    timeBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&time_));

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = timeBuffer_->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = strideBytes;

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(passSRVHeap_->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(3, dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    dev_->CreateConstantBufferView(&cbvDesc, heapHandle);
}

void Dx12Wrapper::CreateBoardRootSignature()
{
    HRESULT result = S_OK;
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE range[2] = {};
    D3D12_ROOT_PARAMETER parameter[1] = {};

    range[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0);
    range[1] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[0], 2, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);

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
    Dx12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), 
        IID_PPV_ARGS(boardRootSig_.GetAddressOf()));
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

bool Dx12Wrapper::CreateBackBufferView()
{
    DXGI_SWAP_CHAIN_DESC1 swDesc = {};
    swapchain_->GetDesc1(&swDesc);
    
    Dx12Helper::CreateDescriptorHeap(dev_, bbRTVHeap_, back_buffer_count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

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
    
    currentBackBuffer_ = 0;

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

    depthBuffer_ = Dx12Helper::CreateTex2DBuffer(dev_, rtvDesc.Width, rtvDesc.Height,
        D3D12_HEAP_TYPE_DEFAULT, 
        depth_resource_format,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, 
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    Dx12Helper::CreateDescriptorHeap(dev_, dsvHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

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

void Dx12Wrapper::CreateShadowMapping()
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

    shadowDepthBuffer_ = Dx12Helper::CreateTex2DBuffer(dev_, 1024, 1024,
        D3D12_HEAP_TYPE_DEFAULT,
        DXGI_FORMAT_R32_TYPELESS,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    Dx12Helper::CreateDescriptorHeap(dev_, shadowDSVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    dev_->CreateDepthStencilView(
        shadowDepthBuffer_.Get(),
        &dsvDesc,
        shadowDSVHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void Dx12Wrapper::CreateShadowRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[1] = {};
    // transform and bone
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        2,                                      // number of descriptors
        0);                                     // base shader register


    D3D12_ROOT_PARAMETER rootParam[1] = {};
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[0], 1, &range[0],D3D12_SHADER_VISIBILITY_VERTEX);

    rtSigDesc.pParameters = rootParam;
    rtSigDesc.NumParameters = _countof(rootParam);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errBlob;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    Dx12Helper::OutputFromErrorBlob(errBlob);
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
    Dx12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Other set up

    // Depth/Stencil
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.NodeMask = 0;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // Output set up
    // Blend
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // Root Signature
    psoDesc.pRootSignature = shadowRootSig_.Get();

    result = dev_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(shadowPipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));

}

void Dx12Wrapper::RenderToShadowDepthBuffer()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeap(shadowDSVHeap_->GetCPUDescriptorHandleForHeapStart());
    cmdList_->OMSetRenderTargets(0, nullptr, false, &dsvHeap);
    cmdList_->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    cmdList_->SetPipelineState(shadowPipeline_.Get());
    cmdList_->SetGraphicsRootSignature(shadowRootSig_.Get());

    CD3DX12_VIEWPORT vp(shadowDepthBuffer_.Get());
    cmdList_->RSSetViewports(1, &vp);

    auto desc = shadowDepthBuffer_->GetDesc();
    CD3DX12_RECT rc(0, 0, desc.Width, desc.Height);
    cmdList_->RSSetScissorRects(1, &rc);

    for (auto& model : pmdModelList_)
    {
        model->SetTransformGraphicPipeline(cmdList_);
        model->SetInputAssembler(cmdList_);
        cmdList_->DrawIndexedInstanced(model->GetIndicesSize(), 1, 0, 0, 0);
    }

    // After draw to shadow buffer, change its state from DSV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            shadowDepthBuffer_.Get(),                       // resource buffer
            D3D12_RESOURCE_STATE_DEPTH_WRITE,               // state before
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);    // state after
    cmdList_->ResourceBarrier(1, &barrier);
}

void Dx12Wrapper::CreatePlane()
{
    CreatePlaneBuffer();
    CreateDescriptorForPrimitive();
    CreatePlaneRootSignature();
    CreatePlanePipeLine();
}

void Dx12Wrapper::CreatePlaneBuffer()
{
    /*PrimitiveVertex verts[] = {
        {{-10.0f,0.0f,-10.0f}},
        {{10.0f,0.0f,-10.0f}},
        {{10.0f,0.0f,10.0f}},
        {{-10.0f,0.0f,10.0f}},
        {{10.0f,50.0f,10.0f}},
        {{-10.0f,50.0f,10.0f}},
    };*/

    PrimitiveVertex verts[] = {
        {{10.0f,0.0f,-10.0f}},
        {{-10.0f,0.0f,-10.0f}},
        {{10.0f,0.0f,10.0f}},
        {{-10.0f,0.0f,10.0f}},
        {{10.0f,50.0f,10.0f}},
        {{-10.0f,50.0f,10.0f}},
    };

    auto byteSize = sizeof(PrimitiveVertex) * _countof(verts);
    planeVB_ = Dx12Helper::CreateBuffer(dev_, byteSize);
    
    planeVBV_.BufferLocation = planeVB_->GetGPUVirtualAddress();
    planeVBV_.SizeInBytes = byteSize;
    planeVBV_.StrideInBytes = sizeof(PrimitiveVertex);

    PrimitiveVertex* mappedVertices = nullptr;
    Dx12Helper::ThrowIfFailed(planeVB_->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices)));
    std::copy(std::begin(verts), std::end(verts), mappedVertices);
    planeVB_->Unmap(0, nullptr);
}

void Dx12Wrapper::CreatePlaneRootSignature()
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
        1,                                      // number of descriptors
        0);                                     // base shader register

    range[1] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // range type
        1,                                      // number of descriptors
        0);                                     // base shader register

    D3D12_ROOT_PARAMETER parameter[1] = {};
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[0], 2, &range[0], D3D12_SHADER_VISIBILITY_ALL);

    rtSigDesc.pParameters = parameter;
    rtSigDesc.NumParameters = _countof(parameter);

    D3D12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
    CD3DX12_STATIC_SAMPLER_DESC::Init(samplerDesc[0], 0,
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,
        D3D12_TEXTURE_ADDRESS_MODE_BORDER);
    
    rtSigDesc.pStaticSamplers = samplerDesc;
    rtSigDesc.NumStaticSamplers = _countof(samplerDesc);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errBlob;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    Dx12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = dev_->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
        IID_PPV_ARGS(planeRootSig_.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void Dx12Wrapper::CreatePlanePipeLine()
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
        L"shader/primitiveVS.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "primitiveVS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    Dx12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

    // Pixel Shader
    ComPtr<ID3DBlob> psBlob;
    result = D3DCompileFromFile(
        L"shader/primitivePS.hlsl",                              // path to shader file
        nullptr,                                        // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
        "primitivePS",                                           // entry name
        "ps_5_1",                                       // shader version
        0, 0,                                            // Flag1, Flag2 (unknown)
        psBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    Dx12Helper::OutputFromErrorBlob(errBlob);
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

    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // Root Signature
    psoDesc.pRootSignature = planeRootSig_.Get();

    result = dev_->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(planePipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void Dx12Wrapper::RenderPrimitive()
{
    cmdList_->SetPipelineState(planePipeline_.Get());
    cmdList_->SetGraphicsRootSignature(planeRootSig_.Get());
    cmdList_->SetDescriptorHeaps(1, primitiveHeap_.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE primitiveHeap(primitiveHeap_->GetGPUDescriptorHandleForHeapStart());
    cmdList_->SetGraphicsRootDescriptorTable(0, primitiveHeap);

    cmdList_->IASetVertexBuffers(0, 1, &planeVBV_);
    cmdList_->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList_->DrawInstanced(6, 1, 0, 0);

}

void Dx12Wrapper::CreateDescriptorForPrimitive()
{
    Dx12Helper::CreateDescriptorHeap(dev_, primitiveHeap_, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    auto transformBuffer = pmdModelList_[0]->GetTransformBuffer();

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(primitiveHeap_->GetCPUDescriptorHandleForHeapStart());
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = transformBuffer->GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = transformBuffer->GetDesc().Width;
    dev_->CreateConstantBufferView(&cbvDesc, heapHandle);

    heapHandle.Offset(1, dev_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    dev_->CreateShaderResourceView(shadowDepthBuffer_.Get(), &srvDesc, heapHandle);
}

void Dx12Wrapper::WaitForGPU()
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
    CreateShadowResourceForPostEffect();
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
    CreateBackBufferView();
    CreateDepthBuffer();  
    CreatePostEffectTexture();
    CreateShadowMapping();
    CreateDefaultTexture();
    CreatePMDModel();
    CreatePlane();
    
    return true;
}

void Dx12Wrapper::CreatePMDModel()
{
    pmdModelList_.emplace_back(std::make_shared<PMDModel>(dev_));
    pmdModelList_[0]->GetDefaultTexture(whiteTexture_, blackTexture_, gradTexture_);
    pmdModelList_[0]->LoadPMD(model1_path);
    pmdModelList_[0]->CreateModel();
    pmdModelList_[0]->CreateShadowDepthView(shadowDepthBuffer_);
    pmdModelList_[0]->LoadMotion(motion1_path);

    /*pmdModelList_.emplace_back(std::make_shared<PMDModel>(dev_));
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
    pmdModelList_[2]->Transform(XMMatrixTranslation(-15, 0, 5));*/
}

bool Dx12Wrapper::Update(const float& deltaTime)
{
    if (timer <= 0.0f)
    {
        frameNO++;
        timer = animation_speed;
        scalar += 0.05;
        angle = 0.01f;
        for (auto& model : pmdModelList_)
        {
            model->Transform(XMMatrixRotationY(angle));
        }
    }
    timer -= deltaTime;

    scalar = scalar > 5 ? 0.1 : scalar;
    auto test = *time_;
    *time_ = scalar;

    return true;
}

void Dx12Wrapper::SetResourceStateForNextFrame()
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtTexture_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList_->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        shadowDepthBuffer_.Get(),                  // resource buffer
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,               // state before
        D3D12_RESOURCE_STATE_DEPTH_WRITE);        // state after
    cmdList_->ResourceBarrier(1, &barrier);

    // change back buffer state to PRESENT to ready to be rendered
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffers_[currentBackBuffer_].Get(),                  // resource buffer
        D3D12_RESOURCE_STATE_RENDER_TARGET,         // state before
        D3D12_RESOURCE_STATE_PRESENT);              // state after
    cmdList_->ResourceBarrier(1, &barrier);
}

bool Dx12Wrapper::Render()
{
    // Clear commands and open
    cmdAlloc_->Reset();
    cmdList_->Reset(cmdAlloc_.Get(), nullptr);

    RenderToShadowDepthBuffer();
    RenderToPostEffectBuffer();
    RenderToBackBuffer();
    SetResourceStateForNextFrame();
    SetBackBufferIndexForNextFrame();

    cmdList_->Close();
    WaitForGPU();

    // screen flip
    swapchain_->Present(0, 0);
    return true;
}

void Dx12Wrapper::Terminate()
{
    //boneBuffer_->Unmap(0, nullptr);
    timeBuffer_->Unmap(0, nullptr);
}
