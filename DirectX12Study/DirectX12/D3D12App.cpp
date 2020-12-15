#include "D3D12App.h"
#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include "../PMDModel/PMDManager.h"
#include "../Geometry/GeometryGenerator.h"

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
    constexpr unsigned int material_descriptor_count = 5;
    //const char* model_path = "resource/PMD/model/桜ミク/mikuXS桜ミク.pmd";
    //const char* model_path = "resource/PMD/model/桜ミク/mikuXS雪ミク.pmd";
    //const char* model_path = "resource/PMD/model/霊夢/reimu_G02.pmd";
    const char* model_path = "Resource/PMD/model/弱音ハク.pmd";
    const char* model1_path = "Resource/PMD/model/初音ミク.pmd";
    const char* model3_path = "Resource/PMD/model/柳生/柳生Ver1.12SW.pmd";
    const char* model2_path = "Resource/PMD/model/hibiki/我那覇響v1_グラビアミズギ.pmd";

    const char* motion1_path = "Resource/VMD/ヤゴコロダンス.vmd";
    const char* motion2_path = "Resource/VMD/swing2.vmd";

    float g_scalar = 0.1;
    constexpr float scale_speed = 1;
}

void D3D12App::CreateDefaultTexture()
{
    HRESULT result = S_OK;

    // 転送先
    m_whiteTexture = D12Helper::CreateTex2DBuffer(m_device.Get(),4, 4);
    m_blackTexture = D12Helper::CreateTex2DBuffer(m_device.Get(),4, 4);
    m_gradTexture = D12Helper::CreateTex2DBuffer(m_device.Get(),4, 4);

    struct Color
    {
        uint8_t r, g, b, a;
        Color() :r(0), g(0), b(0), a(0) {};
        Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha) :
            r(red), g(green), b(blue), a(alpha) {};
    };

    std::vector<Color> whiteTex(4 * 4);
    std::fill(whiteTex.begin(), whiteTex.end(), Color(0xff, 0xff, 0xff, 0xff));
    const std::string whiteTexStr = "white texture";
    m_updateBuffers.Add(whiteTexStr, whiteTex.data(), sizeof(whiteTex[0]) * 4, whiteTex.size() * 4);
    D12Helper::UpdateDataToTextureBuffer(m_device.Get(), m_cmdList.Get(), m_whiteTexture, 
        m_updateBuffers.GetBuffer(whiteTexStr),
        m_updateBuffers.GetSubresource(whiteTexStr));

    std::vector<Color> blackTex(4 * 4);
    std::fill(blackTex.begin(), blackTex.end(), Color(0x00, 0x00, 0x00, 0xff));
    const std::string blackTexStr = "black texture";
    m_updateBuffers.Add(blackTexStr, whiteTex.data(), sizeof(whiteTex[0]) * 4, whiteTex.size() * 4);
    D12Helper::UpdateDataToTextureBuffer(m_device.Get(), m_cmdList.Get(), m_whiteTexture,
        m_updateBuffers.GetBuffer(blackTexStr),
        m_updateBuffers.GetSubresource(blackTexStr));
    
    std::vector<Color> gradTex(4 * 4);
    gradTex.resize(256);
    for (size_t i = 0; i < 256; ++i)
        std::fill_n(&gradTex[i], 1, Color(255 - i, 255 - i, 255 - i, 0xff));
    const std::string gradTexStr = "gradiation texture";
    m_updateBuffers.Add(gradTexStr, whiteTex.data(), sizeof(whiteTex[0]) * 4, whiteTex.size() * 4);
    D12Helper::UpdateDataToTextureBuffer(m_device.Get(), m_cmdList.Get(), m_whiteTexture,
        m_updateBuffers.GetBuffer(gradTexStr),
        m_updateBuffers.GetSubresource(gradTexStr));
    
}

void D3D12App::CreateViewForRenderTargetTexture()
{
    HRESULT result = S_OK;
    auto rsDesc = m_backBuffer[0]->GetDesc();
    float color[] = { 0.f,0.0f,0.0f,1.0f };
    auto clearValue = CD3DX12_CLEAR_VALUE(rsDesc.Format, color);
    rtTexture_ = D12Helper::CreateTex2DBuffer(m_device.Get(),
        rsDesc.Width, rsDesc.Height, rsDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(),passRTVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = rsDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    m_device->CreateRenderTargetView(rtTexture_.Get(), &rtvDesc, passRTVHeap_->GetCPUDescriptorHandleForHeapStart());

    // SRV
    D12Helper::CreateDescriptorHeap(m_device.Get(),passSRVHeap_, 3, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = rsDesc.Format;
    auto heapHandle = passSRVHeap_->GetCPUDescriptorHandleForHeapStart();
    m_device->CreateShaderResourceView(rtTexture_.Get(), &srvDesc, heapHandle);
}

void D3D12App::CreateBoardPolygonVertices()
{
    XMFLOAT3 vert[] = { {-1.0f,-1.0f,0},                // bottom left
                        {1.0f,-1.0f,0},                 // bottom right
                        {-1.0f,1.0f,0},                 // top left
                        {1.0f,1.0f,0} };                // top right
    boardPolyVert_ = D12Helper::CreateBuffer(m_device.Get(),sizeof(vert));

    XMFLOAT3* mappedData = nullptr;
    auto result = boardPolyVert_->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    std::copy(std::begin(vert), std::end(vert), mappedData);
    boardPolyVert_->Unmap(0, nullptr);

    boardVBV_.BufferLocation = boardPolyVert_->GetGPUVirtualAddress();
    boardVBV_.SizeInBytes = sizeof(vert);
    boardVBV_.StrideInBytes = sizeof(XMFLOAT3);

}

void D3D12App::CreateBoardPipeline()
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
        L"Shader/boardVS.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "boardVS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    D12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());

    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    // Pixel Shader
    ComPtr<ID3DBlob> psBlob;
    result = D3DCompileFromFile(
        L"Shader/boardPS.hlsl",                              // path to shader file
        nullptr,                                        // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,              // include object
        "boardPS",                                           // entry name
        "ps_5_1",                                       // shader version
        0, 0,                                            // Flag1, Flag2 (unknown)
        psBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    D12Helper::OutputFromErrorBlob(errBlob);
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

    result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(boardPipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));

}

void D3D12App::RenderToPostEffectBuffer()
{
    auto wsize = Application::Instance().GetWindowSize();
    CD3DX12_VIEWPORT vp(m_backBuffer[m_currentBackBuffer].Get());
    m_cmdList->RSSetViewports(1, &vp);

    CD3DX12_RECT rc(0, 0, wsize.width, wsize.height);
    m_cmdList->RSSetScissorRects(1, &rc);

    float peDefaultColor[4] = { 1.0f,0.0f,0.0f,0.0f };
    auto postEffectHeap = passRTVHeap_->GetCPUDescriptorHandleForHeapStart();
    auto dsvHeap = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_cmdList->OMSetRenderTargets(1, &postEffectHeap, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_cmdList->ClearRenderTargetView(postEffectHeap, peDefaultColor, 0, nullptr);

    RenderPrimitive();
    m_pmdManager->Render(m_cmdList.Get());
    EffekseerRender();

    // Set resource state of postEffectTexture from RTV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtTexture_.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cmdList->ResourceBarrier(1, &barrier);
}

void D3D12App::RenderToBackBuffer()
{
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_backBuffer[m_currentBackBuffer].Get(),      // resource buffer
            D3D12_RESOURCE_STATE_PRESENT,               // state before
            D3D12_RESOURCE_STATE_RENDER_TARGET);        // state after
    m_cmdList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE bbRTVHeap(m_bbRTVHeap->GetCPUDescriptorHandleForHeapStart());
    const auto rtvIncreSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    bbRTVHeap.Offset(m_currentBackBuffer, rtvIncreSize);

    float bbDefaultColor[] = { 0.0f,0.0f,0.0f,1.f };
    m_cmdList->OMSetRenderTargets(1, &bbRTVHeap, false, nullptr);
    m_cmdList->ClearRenderTargetView(bbRTVHeap, bbDefaultColor, 0, nullptr);

    m_cmdList->SetPipelineState(boardPipeline_.Get());
    m_cmdList->SetGraphicsRootSignature(boardRootSig_.Get());

    m_cmdList->SetDescriptorHeaps(1, passSRVHeap_.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE shaderHeap(passSRVHeap_->GetGPUDescriptorHandleForHeapStart());
    m_cmdList->SetGraphicsRootDescriptorTable(0, shaderHeap);

    m_cmdList->IASetVertexBuffers(0, 1, &boardVBV_);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_cmdList->DrawInstanced(4, 1, 0, 0);

}

void D3D12App::SetBackBufferIndexForNextFrame()
{
    // Swap back buffer index to next frame use
    m_currentBackBuffer = (m_currentBackBuffer + 1) % back_buffer_count;
}

void D3D12App::EffekseerInit()
{
    // Create renderer
    auto backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_effekRenderer = EffekseerRendererDX12::Create(m_device.Get(), m_cmdQue.Get(), 2, &backBufferFormat, 1,
        DXGI_FORMAT_UNKNOWN, false, 8000);

    // Create memory pool
    m_effekPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(m_effekRenderer);

    // Create commandList
    m_effekCmdList = EffekseerRendererDX12::CreateCommandList(m_effekRenderer, m_effekPool);

    // Create manager
    m_effekManager = Effekseer::Manager::Create(8000);

    // Set renderer module for manager
    m_effekManager->SetSpriteRenderer(m_effekRenderer->CreateSpriteRenderer());
    m_effekManager->SetRibbonRenderer(m_effekRenderer->CreateRibbonRenderer());
    m_effekManager->SetRingRenderer(m_effekRenderer->CreateRingRenderer());
    m_effekManager->SetTrackRenderer(m_effekRenderer->CreateTrackRenderer());
    m_effekManager->SetModelRenderer(m_effekRenderer->CreateModelRenderer());

    // Set loader for manager
    m_effekManager->SetTextureLoader(m_effekRenderer->CreateTextureLoader());
    m_effekManager->SetMaterialLoader(m_effekRenderer->CreateMaterialLoader());
    m_effekManager->SetModelLoader(m_effekRenderer->CreateModelLoader());

    // Load effect from file path
    const EFK_CHAR* effect_path = u"Resource/Effekseer/Laser01.efk";
    m_effekEffect = Effekseer::Effect::Create(m_effekManager, effect_path);

    auto& app = Application::Instance();
    auto screenSize = app.GetWindowSize();

    auto viewPos = Effekseer::Vector3D(10.0f, 10.0f, 10.0f);

    m_effekRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovRH(XM_PIDIV4, app.GetAspectRatio(), 1.0f, 500.0f));

    m_effekRenderer->SetCameraMatrix(
        Effekseer::Matrix44().LookAtRH(viewPos, Effekseer::Vector3D(0.0f, 0.0f, 0.0f), Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));
}

void D3D12App::EffekseerUpdate(const float& deltaTime)
{
    constexpr float effect_animation_time = 10.0f;      // 10 seconds
    static float s_timer = 0.0f;
    static auto s_angle = 0.0f;

    if (s_timer <= 0.0f)
    {
        m_effekHandle = m_effekManager->Play(m_effekEffect, 0, 0, 0);
        s_timer = effect_animation_time;
    }
    
    s_angle += 1.f * deltaTime;
    m_effekManager->AddLocation(m_effekHandle, Effekseer::Vector3D(0.1f * deltaTime, 0.0f, 0.0f));
    m_effekManager->SetRotation(m_effekHandle, Effekseer::Vector3D(0.0f, 1.0f, 0.0f), s_angle);
    //m_effekManager->StopEffect(m_effekHandle);

    m_effekManager->Update(deltaTime);
    s_timer -= deltaTime;
    
}

void D3D12App::EffekseerRender()
{
    // Set up frame
    m_effekPool->NewFrame();
    EffekseerRendererDX12::BeginCommandList(m_effekCmdList, m_cmdList.Get());
    m_effekRenderer->SetCommandList(m_effekCmdList);
    // Render
    m_effekRenderer->BeginRendering();
    m_effekManager->Draw();
    m_effekRenderer->EndRendering();
    // End frame
    m_effekRenderer->SetCommandList(nullptr);
    EffekseerRendererDX12::EndCommandList(m_effekCmdList);
}

void D3D12App::EffekseerTerminate()
{
    ES_SAFE_RELEASE(m_effekEffect);

    m_effekManager->Destroy();

    ES_SAFE_RELEASE(m_effekCmdList);
    ES_SAFE_RELEASE(m_effekPool);

    m_effekRenderer->Destroy();
}

void D3D12App::CreateNormalMapTexture()
{
    normalMapTex_ = D12Helper::CreateTextureFromFilePath(m_device, L"Resource/Image/normalmap3.png");

    HRESULT result = S_OK;
    auto rsDesc = m_backBuffer[0]->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = normalMapTex_.Get()->GetDesc().Format;

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(passSRVHeap_->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateShaderResourceView(normalMapTex_.Get(), &srvDesc, heapHandle);
}

void D3D12App::CreateTimeBuffer()
{
    m_timeBuffer.Create(m_device.Get(),1,true);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_timeBuffer.GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_timeBuffer.SizeInBytes();

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(passSRVHeap_->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(2, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateConstantBufferView(&cbvDesc, heapHandle);
}

void D3D12App::CreateBoardRootSignature()
{
    HRESULT result = S_OK;
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE range[2] = {};
    D3D12_ROOT_PARAMETER parameter[1] = {};

    range[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
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
    D12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), 
        IID_PPV_ARGS(boardRootSig_.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::CreateCommandFamily()
{
    const auto command_list_type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    HRESULT result = S_OK;
    D3D12_COMMAND_QUEUE_DESC cmdQdesc = {};
    cmdQdesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    cmdQdesc.NodeMask = 0;
    cmdQdesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmdQdesc.Type = command_list_type;
    result = m_device->CreateCommandQueue(&cmdQdesc, IID_PPV_ARGS(m_cmdQue.GetAddressOf()));
    assert(SUCCEEDED(result));

    result = m_device->CreateCommandAllocator(command_list_type,
        IID_PPV_ARGS(m_cmdAlloc.GetAddressOf()));
    assert(SUCCEEDED(result));

    result = m_device->CreateCommandList(0, command_list_type,
        m_cmdAlloc.Get(),
        nullptr,
        IID_PPV_ARGS(m_cmdList.GetAddressOf()));
    assert(SUCCEEDED(result));

    m_cmdList->Close();
}

void D3D12App::CreateSwapChain(const HWND& hwnd)
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
    auto result = m_dxgi->CreateSwapChainForHwnd(
        m_cmdQue.Get(),
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        swapchain1.ReleaseAndGetAddressOf());
    assert(SUCCEEDED(result));
    result = swapchain1.As(&m_swapchain);
    assert(SUCCEEDED(result));
}

bool D3D12App::CreateBackBufferView()
{
    DXGI_SWAP_CHAIN_DESC1 swDesc = {};
    m_swapchain->GetDesc1(&swDesc);
    
    D12Helper::CreateDescriptorHeap(m_device.Get(), m_bbRTVHeap, back_buffer_count, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_backBuffer.resize(back_buffer_count);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = swDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    auto heap = m_bbRTVHeap->GetCPUDescriptorHandleForHeapStart();
    const auto increamentSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (int i = 0; i < back_buffer_count; ++i)
    {
        m_swapchain->GetBuffer(i, IID_PPV_ARGS(m_backBuffer[i].GetAddressOf()));
        m_device->CreateRenderTargetView(m_backBuffer[i].Get(),
            &rtvDesc,
            heap);
        heap.ptr += increamentSize;
    }
    
    m_currentBackBuffer = 0;

    return true;
}

bool D3D12App::CreateDepthBuffer()
{
    HRESULT result = S_OK;
    auto rtvDesc = m_backBuffer[0]->GetDesc();
    const auto depth_resource_format = DXGI_FORMAT_D32_FLOAT;

    CD3DX12_CLEAR_VALUE clearValue(
        depth_resource_format                       // format
        ,1.0f                                       // depth
        ,0);                                        // stencil

    m_depthBuffer = D12Helper::CreateTex2DBuffer(m_device.Get(),
        rtvDesc.Width, rtvDesc.Height, depth_resource_format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(), m_dsvHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = depth_resource_format;
    m_device->CreateDepthStencilView(
        m_depthBuffer.Get(),
        &dsvDesc,
        m_dsvHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void D3D12App::CreateShadowMapping()
{
    CreateShadowDepthBuffer();
    CreateShadowRootSignature();
    CreateShadowPipelineState();
}

bool D3D12App::CreateShadowDepthBuffer()
{
    HRESULT result = S_OK;
    auto rtvDesc = m_backBuffer[0]->GetDesc();
    const auto depth_resource_format = DXGI_FORMAT_D32_FLOAT;

    CD3DX12_CLEAR_VALUE clearValue(
        depth_resource_format                       // format
        , 1.0f                                       // depth
        , 0);                                        // stencil

    m_shadowDepthBuffer = D12Helper::CreateTex2DBuffer(m_device.Get(),
        1024, 1024, DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(), shadowDSVHeap_, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    m_device->CreateDepthStencilView(
        m_shadowDepthBuffer.Get(),
        &dsvDesc,
        shadowDSVHeap_->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void D3D12App::CreateShadowRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[2] = {};
    D3D12_ROOT_PARAMETER rootParam[2] = {};

    // World pass constant
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        1,                                      // number of descriptors
        0);                                     // base shader register
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[0], 1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);

    //World pass constant
    range[1] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        1,                                      // number of descriptors
        1);                                     // base shader register
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1], 1, &range[1], D3D12_SHADER_VISIBILITY_VERTEX);
   
    rtSigDesc.pParameters = rootParam;
    rtSigDesc.NumParameters = _countof(rootParam);

    ComPtr<ID3DBlob> rootSigBlob;
    ComPtr<ID3DBlob> errBlob;
    result = D3D12SerializeRootSignature(&rtSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1_0,             //※ 
        &rootSigBlob,
        &errBlob);
    D12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(), 
        IID_PPV_ARGS(shadowRootSig_.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::CreateShadowPipelineState()
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
        L"Shader/ShadowVS.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "ShadowVS",                                               // entry name
        "vs_5_1",                                           // shader version
        0, 0,                                               // Flag1, Flag2 (unknown)
        vsBlob.GetAddressOf(),
        errBlob.GetAddressOf());
    D12Helper::OutputFromErrorBlob(errBlob);
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

    result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(shadowPipeline_.GetAddressOf()));
    assert(SUCCEEDED(result));

}

void D3D12App::RenderToShadowDepthBuffer()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeap(shadowDSVHeap_->GetCPUDescriptorHandleForHeapStart());
    m_cmdList->OMSetRenderTargets(0, nullptr, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    m_cmdList->SetPipelineState(shadowPipeline_.Get());
    m_cmdList->SetGraphicsRootSignature(shadowRootSig_.Get());

    CD3DX12_VIEWPORT vp(m_shadowDepthBuffer.Get());
    m_cmdList->RSSetViewports(1, &vp);

    auto desc = m_shadowDepthBuffer->GetDesc();
    CD3DX12_RECT rc(0, 0, desc.Width, desc.Height);
    m_cmdList->RSSetScissorRects(1, &rc);

    m_pmdManager->RenderDepth(m_cmdList.Get());

    // After draw to shadow buffer, change its state from DSV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_shadowDepthBuffer.Get(),                       // resource buffer
            D3D12_RESOURCE_STATE_DEPTH_WRITE,               // state before
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);    // state after
    m_cmdList->ResourceBarrier(1, &barrier);
}

void D3D12App::CreatePrimitive()
{
    CreatePrimitiveBuffer();
    CreateDescriptorForPrimitive();
    CreatePrimitiveRootSignature();
    CreatePrimitivePipeLine();
}

void D3D12App::CreatePrimitiveBuffer()
{
    CreatePrimitiveVertexBuffer();
    CreatePrimitiveIndexBuffer();
}

namespace
{
    auto g_primitive = GeometryGenerator::CreateSphere(10.f,20,20);
}

void D3D12App::CreatePrimitiveVertexBuffer()
{
    auto byteSize = sizeof(g_primitive.vertices[0]) * g_primitive.vertices.size();
    m_primitiveVB = D12Helper::CreateBuffer(m_device.Get(), byteSize);
    
    m_primitiveVBV.BufferLocation = m_primitiveVB->GetGPUVirtualAddress();
    m_primitiveVBV.SizeInBytes = byteSize;
    m_primitiveVBV.StrideInBytes = sizeof(g_primitive.vertices[0]);

    auto type = g_primitive.vertices[0];
    decltype(type)* mappedVertices = nullptr;

    D12Helper::ThrowIfFailed(m_primitiveVB->Map(0, nullptr, reinterpret_cast<void**>(&mappedVertices)));
    std::copy(g_primitive.vertices.begin(), g_primitive.vertices.end(), mappedVertices);
    m_primitiveVB->Unmap(0, nullptr);

}

void D3D12App::CreatePrimitiveIndexBuffer()
{
    auto byteSize = sizeof(g_primitive.indices[0]) * g_primitive.indices.size();
    m_primitiveIB = D12Helper::CreateBuffer(m_device.Get(), byteSize);

    uint16_t* mappedData = nullptr;
    D12Helper::ThrowIfFailed(m_primitiveIB->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));
    std::copy(g_primitive.indices.begin(), g_primitive.indices.end(), mappedData);
    m_primitiveIB->Unmap(0, nullptr);

    m_primitiveIBV.BufferLocation = m_primitiveIB->GetGPUVirtualAddress();
    m_primitiveIBV.SizeInBytes = byteSize;
    m_primitiveIBV.Format = DXGI_FORMAT_R16_UINT;
}

void D3D12App::CreatePrimitiveRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[2] = {};

    // world pass constant
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        1,                                      // number of descriptors
        0);                                     // base shader register

    // shadow depth buffer
    range[1] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV,        // range type
        1,                                      // number of descriptors
        0);                                     // base shader register

    // object constant

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
    D12Helper::OutputFromErrorBlob(errBlob);
    assert(SUCCEEDED(result));
    result = m_device->CreateRootSignature(0, rootSigBlob->GetBufferPointer(), rootSigBlob->GetBufferSize(),
        IID_PPV_ARGS(m_primitiveRootSig.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::CreatePrimitivePipeLine()
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
    "NORMAL",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0 
    },
{
    "TANGENT",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0
    },
    {
    "TEXCOORD",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0
    },
    };

    // Input Assembler
    psoDesc.InputLayout.NumElements = _countof(layout);
    psoDesc.InputLayout.pInputElementDescs = layout;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    // Vertex Shader
    ComPtr<ID3DBlob> vsBlob = D12Helper::CompileShaderFromFile(L"Shader/primitiveVS.hlsl", "primitiveVS", "vs_5_1");
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    // Rasterizer
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FrontCounterClockwise = true;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    // Pixel Shader
    ComPtr<ID3DBlob> psBlob = D12Helper::CompileShaderFromFile(L"Shader/primitivePS.hlsl", "primitivePS", "ps_5_1");
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
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    // Root Signature
    psoDesc.pRootSignature = m_primitiveRootSig.Get();
    result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_primitivePipeline.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::RenderPrimitive()
{
    m_cmdList->SetPipelineState(m_primitivePipeline.Get());
    m_cmdList->SetGraphicsRootSignature(m_primitiveRootSig.Get());

    m_cmdList->IASetVertexBuffers(0, 1, &m_primitiveVBV);
    m_cmdList->IASetIndexBuffer(&m_primitiveIBV);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    
    m_cmdList->SetDescriptorHeaps(1, m_primitiveHeap.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE primitiveHeap(m_primitiveHeap->GetGPUDescriptorHandleForHeapStart());
    m_cmdList->SetGraphicsRootDescriptorTable(0, primitiveHeap);
    m_cmdList->DrawIndexedInstanced(g_primitive.indices.size(), 1, 0, 0, 0);

}

void D3D12App::CreateDescriptorForPrimitive()
{
    D12Helper::CreateDescriptorHeap(m_device.Get(), m_primitiveHeap, 2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_primitiveHeap->GetCPUDescriptorHandleForHeapStart());
    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_worldPCBuffer.GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_worldPCBuffer.SizeInBytes();
    m_device->CreateConstantBufferView(&cbvDesc, heapHandle);

    heapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    m_device->CreateShaderResourceView(m_shadowDepthBuffer.Get(), &srvDesc, heapHandle);
}

void D3D12App::WaitForGPU()
{
    // Set current GPU target fence value to next process frame
    ++m_targetFenceValue;

    // use Signal of Command Que to tell GPU to set current GPU fence value
    // to next process frame, after finished last Execution Draw Call
    m_cmdQue->Signal(m_fence.Get(), m_targetFenceValue);

    // Check GPU current fence value 
    // If GPU current fence value haven't reached target Fence Value
    // Tell CPU to wait until GPU reach current fence
    while (m_fence->GetCompletedValue() < m_targetFenceValue)
    {
        auto fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fence->SetEventOnCompletion(m_targetFenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    };
}

void D3D12App::CreatePostEffectTexture()
{
    CreateBoardPolygonVertices();

    CreateViewForRenderTargetTexture();
    CreateNormalMapTexture();
    CreateTimeBuffer();

    CreateBoardRootSignature();
    CreateBoardPipeline();
}

bool D3D12App::Initialize(const HWND& hwnd)
{
    HRESULT result = S_OK;

    result = CoInitializeEx(0, COINIT_MULTITHREADED);
    assert(SUCCEEDED(result));

#if defined(DEBUG) || defined(_DEBUG)
    //ComPtr<ID3D12Debug> debug;
    //D3D12GetDebugInterface(IID_PPV_ARGS(debug.ReleaseAndGetAddressOf()));
    //debug->EnableDebugLayer();

    result = CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(m_dxgi.ReleaseAndGetAddressOf()));

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
    while (m_dxgi->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
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
        //result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));
        /*-------Use strongest graphics card (adapter) GTX-------*/
        result = D3D12CreateDevice(adapterList[1], fLevel, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));
        if (FAILED(result)) {
            //IDXGIAdapter4* pAdapter;
            //dxgi_->EnumWarpAdapter(IID_PPV_ARGS(&pAdapter));
            //result = D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_1, IID_PPV_ARGS(dev_.ReleaseAndGetAddressOf()));
            //OutputDebugString(L"Feature level not found");
            return false;
        }
    }
   
    CreateCommandFamily();

    m_cmdAlloc->Reset();
    m_cmdList->Reset(m_cmdAlloc.Get(), nullptr);

    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
    m_targetFenceValue = m_fence->GetCompletedValue();
    
    CreateSwapChain(hwnd);
    CreateBackBufferView();
    CreateDepthBuffer();  
    CreateWorldPassConstant();
    CreatePostEffectTexture();
    CreateShadowMapping();
    CreateDefaultTexture();
    CreatePMDModel();
    CreatePrimitive();

    m_cmdList->Close();
    m_cmdQue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList*const*>(m_cmdList.GetAddressOf()));
    WaitForGPU();

    EffekseerInit();

    m_updateBuffers.Clear();
    m_pmdManager->ClearSubresources();

    return true;
}

bool D3D12App::CreateWorldPassConstant()
{
    HRESULT result = S_OK;

    D12Helper::CreateDescriptorHeap(m_device.Get(), m_worldPassConstantHeap,
        2, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    m_worldPCBuffer.Create(m_device.Get(), 1, true);

    auto& mappedData = m_worldPCBuffer.MappedData();

    auto wSize = Application::Instance().GetWindowSize();

    // camera array (view)
    XMMATRIX viewproj = XMMatrixLookAtRH(
        { 10.0f, 10.0f, 10.0f, 1.0f },
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f });

    // projection array
    viewproj *= XMMatrixPerspectiveFovRH(XM_PIDIV2,
        static_cast<float>(wSize.width) / static_cast<float>(wSize.height),
        0.1f,
        300.0f);

    mappedData.viewproj = viewproj;
    XMVECTOR plane = { 0,1,0,0 };
    XMVECTOR light = { -1,1,-1,0 };
    mappedData.lightPos = light;
    mappedData.shadow = XMMatrixShadow(plane, light);

    XMVECTOR lightPos = { -10,30,30,1 };
    XMVECTOR targetPos = { 10,0,0,1 };
    auto screenSize = Application::Instance().GetWindowSize();
    float aspect = static_cast<float>(screenSize.width) / screenSize.height;
    mappedData.lightViewProj = XMMatrixLookAtRH(lightPos, targetPos, { 0,1,0,0 }) *
        XMMatrixOrthographicRH(30.f, 30.f, 0.1f, 300.f);

    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
    cbvDesc.BufferLocation = m_worldPCBuffer.GetGPUVirtualAddress();
    cbvDesc.SizeInBytes = m_worldPCBuffer.SizeInBytes();
    auto heapPos = m_worldPassConstantHeap->GetCPUDescriptorHandleForHeapStart();
    m_device->CreateConstantBufferView(&cbvDesc, heapPos);

    return true;
}

void D3D12App::CreatePMDModel()
{
    m_pmdManager = std::make_unique<PMDManager>();

    m_pmdManager->SetDevice(m_device.Get());
    m_pmdManager->SetDefaultBuffer(m_whiteTexture.Get(), m_blackTexture.Get(), m_gradTexture.Get());
    m_pmdManager->SetWorldPassConstant(m_worldPCBuffer.Resource(), m_worldPCBuffer.SizeInBytes());
    m_pmdManager->SetWorldShadowMap(m_shadowDepthBuffer.Get());
    m_pmdManager->CreateModel("Miku", model1_path);
    m_pmdManager->CreateModel("Hibiki", model2_path);
    m_pmdManager->CreateAnimation("Dancing", motion1_path);

    assert(m_pmdManager->Init(m_cmdList.Get()));
    assert(m_pmdManager->Play("Miku", "Dancing"));

    return;
}

bool D3D12App::Update(const float& deltaTime)
{
    // Effekseert
    EffekseerUpdate(deltaTime);
    m_pmdManager->Update(deltaTime);

    static float s_angle = 0.0f;
    s_angle = 0.1f;
    
    static XMVECTOR viewpos = { 10.0f, 10.0f, 10.0f, 1.0f };
    static float movespeed = 10.f;

    // Test
    auto translate = XMMatrixTranslation(5.0f * deltaTime, 0.0f, 0.0f);
    if (m_keyboard.IsPressed('M'))
        

    s_angle = 0.5f * deltaTime;
    auto rotate = XMMatrixRotationY(s_angle);
    if (m_keyboard.IsPressed('R'))
    {
        
    }

    if (m_keyboard.IsPressed(VK_LEFT))
        viewpos.m128_f32[0] -= movespeed * deltaTime;
    if (m_keyboard.IsPressed(VK_RIGHT))
        viewpos.m128_f32[0] += movespeed * deltaTime;

    if(m_keyboard.IsPressed(VK_UP))
        viewpos.m128_f32[1] += movespeed * deltaTime;
    if(m_keyboard.IsPressed(VK_DOWN))
        viewpos.m128_f32[1] -= movespeed * deltaTime;

    auto wSize = Application::Instance().GetWindowSize();

    // camera array (view)
    XMMATRIX viewproj = XMMatrixLookAtRH(
        viewpos,
        { 0.0f, 0.0f, 0.0f, 1.0f },
        { 0.0f, 1.0f, 0.0f, 0.0f });

    // projection array
    viewproj *= XMMatrixPerspectiveFovRH(XM_PIDIV2,
        static_cast<float>(wSize.width) / static_cast<float>(wSize.height),
        0.1f,
        300.0f);

    auto& worldPassData = m_worldPCBuffer.MappedData();
    worldPassData.viewproj = viewproj;
        
    g_scalar = g_scalar > 5 ? 0.1 : g_scalar;
    m_timeBuffer.CopyData(g_scalar);

    return true;
}

void D3D12App::SetResourceStateForNextFrame()
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        rtTexture_.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_shadowDepthBuffer.Get(),                  // resource buffer
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,               // state before
        D3D12_RESOURCE_STATE_DEPTH_WRITE);        // state after
    m_cmdList->ResourceBarrier(1, &barrier);

    // change back buffer state to PRESENT to ready to be rendered
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_backBuffer[m_currentBackBuffer].Get(),                  // resource buffer
        D3D12_RESOURCE_STATE_RENDER_TARGET,         // state before
        D3D12_RESOURCE_STATE_PRESENT);              // state after
    m_cmdList->ResourceBarrier(1, &barrier);
}

bool D3D12App::Render()
{
    // Clear commands and open
    m_cmdAlloc->Reset();
    m_cmdList->Reset(m_cmdAlloc.Get(), nullptr);

    RenderToShadowDepthBuffer();
    RenderToPostEffectBuffer();
    RenderToBackBuffer();
    SetResourceStateForNextFrame();
    SetBackBufferIndexForNextFrame();

    m_cmdList->Close();
    m_cmdQue->ExecuteCommandLists(1, (ID3D12CommandList* const*)m_cmdList.GetAddressOf());
    WaitForGPU();

    // screen flip
    m_swapchain->Present(0, 0);
    return true;
}

void D3D12App::Terminate()
{
    EffekseerTerminate();
}

void D3D12App::ClearKeyState()
{
    m_keyboard.Reset();
}

void D3D12App::OnKeyDown(uint8_t keycode)
{
    m_keyboard.OnKeyDown(keycode);
}

void D3D12App::OnKeyUp(uint8_t keycode)
{
    m_keyboard.OnKeyUp(keycode);
}

void D3D12App::OnMouseMove(int x, int y)
{
    m_mouse.OnMove(x, y);
}

void D3D12App::OnMouseRightDown(int x, int y)
{
    m_mouse.OnRightDown(x, y);
}

void D3D12App::OnMouseRightUp(int x, int y)
{
    m_mouse.OnRightUp(x, y);
}

void D3D12App::OnMouseLeftDown(int x, int y)
{
    m_mouse.OnLeftDown(x, y);
}

void D3D12App::OnMouseLeftUp(int x, int y)
{
    m_mouse.OnLeftUp(x, y);
}
