#include "D3D12App.h"

#include <iostream>
#include <cassert>
#include <algorithm>

#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <DirectXColors.h>

#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include "../Geometry/GeometryGenerator.h"
#include "../PMDModel/PMDManager.h"
#include "../Geometry/PrimitiveManager.h"

#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#pragma comment(lib,"DirectXTex.lib")
#pragma comment(lib,"DxGuid.lib")

using namespace DirectX;

namespace
{
    constexpr unsigned int material_descriptor_count_per_block = 5;
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
    m_whiteTexture = D12Helper::CreateTexture2D(m_device.Get(),4, 4);
    m_blackTexture = D12Helper::CreateTexture2D(m_device.Get(),4, 4);
    m_gradTexture = D12Helper::CreateTexture2D(m_device.Get(),4, 4);

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

void D3D12App::CreateRenderTargetTexture()
{
    HRESULT result = S_OK;
    auto rsDesc = m_backBuffer[0]->GetDesc();
    float color[] = { 0.f,0.0f,0.0f,1.0f };
    auto clearValue = CD3DX12_CLEAR_VALUE(rsDesc.Format, color);
    m_rtTexture = D12Helper::CreateTexture2D(m_device.Get(),
        rsDesc.Width, rsDesc.Height, rsDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    m_rtNormalTexture = D12Helper::CreateTexture2D(m_device.Get(),
        rsDesc.Width, rsDesc.Height, rsDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(),m_boardRTVHeap, 2, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_boardRTVHeap->GetCPUDescriptorHandleForHeapStart());
    // Main render target
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = rsDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    m_device->CreateRenderTargetView(m_rtTexture.Get(), &rtvDesc, rtvHeapHandle);

    // Normal render target
    rtvHeapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    m_device->CreateRenderTargetView(m_rtNormalTexture.Get(), &rtvDesc, rtvHeapHandle);

    //
    // SRV 5 resources
    //
    // Render target texture
    // Normal render target texture
    // Normal map texture
    // Shadow depth texture
    // View Depth Texture
    D12Helper::CreateDescriptorHeap(m_device.Get(),m_boardSRVHeap, 5, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = rsDesc.Format;

    // 1st SRV 
    // main render target
    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_boardSRVHeap->GetCPUDescriptorHandleForHeapStart());
    m_device->CreateShaderResourceView(m_rtTexture.Get(), &srvDesc, srvHeapHandle);

    // 2nd SRV
    // normal render target
    const int rt_normal_index = 1;
    srvHeapHandle.Offset(rt_normal_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    m_device->CreateShaderResourceView(m_rtNormalTexture.Get(), &srvDesc, srvHeapHandle);
}

void D3D12App::CreateBoardPolygonVertices()
{
    XMFLOAT3 vert[] = { {-1.0f,-1.0f,0},                // bottom left
                        {1.0f,-1.0f,0},                 // bottom right
                        {-1.0f,1.0f,0},                 // top left
                        {1.0f,1.0f,0} };                // top right
    m_boardPolyVert = D12Helper::CreateBuffer(m_device.Get(),sizeof(vert));

    XMFLOAT3* mappedData = nullptr;
    auto result = m_boardPolyVert->Map(0, nullptr, reinterpret_cast<void**>(&mappedData));
    std::copy(std::begin(vert), std::end(vert), mappedData);
    m_boardPolyVert->Unmap(0, nullptr);

    m_boardVBV.BufferLocation = m_boardPolyVert->GetGPUVirtualAddress();
    m_boardVBV.SizeInBytes = sizeof(vert);
    m_boardVBV.StrideInBytes = sizeof(XMFLOAT3);

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
    psoDesc.RTVFormats[0] = DEFAULT_BACK_BUFFER_FORMAT;
    // Blend
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    // Root Signature
    psoDesc.pRootSignature = m_boardRootSig.Get();

    result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_boardPipeline.GetAddressOf()));
    assert(SUCCEEDED(result));

}

void D3D12App::CreateBoardShadowDepthView()
{
    auto rsDes = m_shadowDepthBuffer->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    /*--------------EASY TO BECOME BUG----------------*/
    // the type of shadow depth buffer is R32_TYPELESS
    // In able to shader resource view to read shadow depth buffer
    // => Set format SRV to DXGI_FORMAT_R32_FLOAT
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    /*-------------------------------------------------*/

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // 4th SRV
    const int shadow_depth_index = 3;
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_boardSRVHeap->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(shadow_depth_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateShaderResourceView(m_shadowDepthBuffer.Get(), &srvDesc, heapHandle);
}

void D3D12App::CreateBoardViewDepthView()
{
    auto rsDes = m_viewDepthBuffer->GetDesc();
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};

    /*--------------EASY TO BECOME BUG----------------*/
    // the type of shadow depth buffer is R32_TYPELESS
    // In able to shader resource view to read shadow depth buffer
    // => Set format SRV to DXGI_FORMAT_R32_FLOAT
    srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
    /*-------------------------------------------------*/

    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    // 5th SRV
    const int view_depth_index = 4;
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_boardSRVHeap->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(view_depth_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateShaderResourceView(m_viewDepthBuffer.Get(), &srvDesc, heapHandle);
}

void D3D12App::RenderToRenderTargetTexture()
{
    auto wsize = Application::Instance().GetWindowSize();
    CD3DX12_VIEWPORT vp(m_backBuffer[m_currentBackBuffer].Get());
    m_cmdList->RSSetViewports(1, &vp);

    CD3DX12_RECT rc(0, 0, wsize.width, wsize.height);
    m_cmdList->RSSetScissorRects(1, &rc);

    float rtTexDefaultColor[4] = { 0.0f,0.0f,0.0f,0.0f };
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtTexHeap(m_boardRTVHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtNormalTexHeap(m_boardRTVHeap->GetCPUDescriptorHandleForHeapStart());
    rtNormalTexHeap.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtHeapHandle[] = { rtTexHeap , rtNormalTexHeap };
    auto dsvHeap = m_dsvHeap->GetCPUDescriptorHandleForHeapStart();
    m_cmdList->OMSetRenderTargets(2, rtHeapHandle, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_cmdList->ClearRenderTargetView(rtTexHeap, rtTexDefaultColor, 0, nullptr);
    m_cmdList->ClearRenderTargetView(rtNormalTexHeap, rtTexDefaultColor, 0, nullptr);

    m_pmdManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_pmdManager->Render(m_cmdList.Get());
    m_primitiveManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_primitiveManager->Render(m_cmdList.Get());

    // Set resource state of postEffectTexture from RTV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtTexture.Get(),
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

    m_cmdList->SetPipelineState(m_boardPipeline.Get());
    m_cmdList->SetGraphicsRootSignature(m_boardRootSig.Get());

    m_cmdList->SetDescriptorHeaps(1, m_boardSRVHeap.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE shaderHeap(m_boardSRVHeap->GetGPUDescriptorHandleForHeapStart());
    m_cmdList->SetGraphicsRootDescriptorTable(0, shaderHeap);

    m_cmdList->IASetVertexBuffers(0, 1, &m_boardVBV);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_cmdList->DrawInstanced(4, 1, 0, 0);

    EffekseerRender();

}

void D3D12App::SetBackBufferIndexForNextFrame()
{
    // Swap back buffer index to next frame use
    m_currentBackBuffer = (m_currentBackBuffer + 1) % DEFAULT_BACK_BUFFER_COUNT;
}

void D3D12App::EffekseerInit()
{
    // Create renderer
    auto backBufferFormat = m_backBuffer[0]->GetDesc().Format;
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
    const EFK_CHAR* effect_path = u"Resource/Effekseer/test.efk";
    m_effekEffect = Effekseer::Effect::Create(m_effekManager, effect_path);

    auto& app = Application::Instance();
    auto screenSize = app.GetWindowSize();

    auto cameraPos = m_camera.GetCameraPosition();
    auto targetPos = m_camera.GetTargetPosition();
    m_effekRenderer->SetCameraMatrix(
        Effekseer::Matrix44().LookAtRH(
            Effekseer::Vector3D(cameraPos.x, cameraPos.y, cameraPos.z),
            Effekseer::Vector3D(targetPos.x, targetPos.y, targetPos.z),
            Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));

    m_effekRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovRH(XM_PIDIV2, app.GetAspectRatio(), 1.0f, 500.0f));
}

void D3D12App::EffekseerUpdate(const float& deltaTime)
{
    constexpr float effect_animation_time = 10.0f;      // 10 seconds
    static float s_timer = 0.0f;
    static auto s_angle = 0.0f;
    static bool s_isPlaying = false;

    if (s_timer <= 0.0f)
    {
        s_timer = effect_animation_time;

        if (!s_isPlaying)
        {
            m_effekHandle = m_effekManager->Play(m_effekEffect, 0, 0, 0);
            s_isPlaying = true;
        }   
    }
    
    s_angle += 1.f * deltaTime;
    m_effekManager->AddLocation(m_effekHandle, Effekseer::Vector3D(1.f * deltaTime, 0.0f, 0.0f));
    m_effekManager->SetRotation(m_effekHandle, Effekseer::Vector3D(0.0f, 1.0f, 0.0f), s_angle);

    if (m_keyboard.IsPressed('S'))
    {
        m_effekManager->StopEffect(m_effekHandle);
        s_isPlaying = false;
    }
    
    m_effekManager->Update(millisecond_per_frame/second_to_millisecond);
    s_timer -= deltaTime;

    auto cameraPos = m_camera.GetCameraPosition();
    auto targetPos = m_camera.GetTargetPosition();
    m_effekRenderer->SetCameraMatrix(
        Effekseer::Matrix44().LookAtRH(
            Effekseer::Vector3D(cameraPos.x, cameraPos.y, cameraPos.z),
            Effekseer::Vector3D(targetPos.x, targetPos.y, targetPos.z),
            Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));
    
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

void D3D12App::UpdateCamera(const float& deltaTime)
{
    if (m_mouse.IsRightPressed())
    {
        auto dx = static_cast<float>(m_mouse.GetPosX() - m_lastMousePos.x);
        auto dy = static_cast<float>(m_mouse.GetPosY() - m_lastMousePos.y);

        m_camera.RotateAroundTarget(dx, dy);
    }
    constexpr float camera_move_speed = 10.0f;
    if (m_keyboard.IsPressed(VK_UP))
    {
        m_camera.DollyInTarget(camera_move_speed * deltaTime);
    }
    if (m_keyboard.IsPressed(VK_DOWN))
    {
        m_camera.DollyOutTarget(camera_move_speed * deltaTime);
    }
    // Update last mouse position
    m_mouse.GetPos(m_lastMousePos.x, m_lastMousePos.y);
    m_camera.Update();

}

void D3D12App::UpdateWorldPassConstant()
{
    auto pMappedData = m_worldPCBuffer.GetHandleMappedData(m_currentFrameResourceIndex);
    pMappedData->ViewPos = m_camera.GetCameraPosition();
    pMappedData->ViewProj = m_camera.GetViewProjectionMatrix();
}

void D3D12App::CreateNormalMapTexture()
{
    m_normalMapTex = D12Helper::CreateTextureFromFilePath(m_device.Get(), L"Resource/Image/normalmap3.png");

    HRESULT result = S_OK;
    auto rsDesc = m_backBuffer[0]->GetDesc();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.PlaneSlice = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // ※
    srvDesc.Format = m_normalMapTex.Get()->GetDesc().Format;

    // 3rd SRV
    const int normal_texture_index = 2;
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_boardSRVHeap->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(normal_texture_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateShaderResourceView(m_normalMapTex.Get(), &srvDesc, heapHandle);
}

void D3D12App::CreateBoardRootSignature()
{
    HRESULT result = S_OK;
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_DESCRIPTOR_RANGE range[1] = {};
    D3D12_ROOT_PARAMETER parameter[1] = {};

    // Main Render target texture 
    // Normal render target texture
    // Normal texture
    // Shadow depth texture
    // View depth texture
    range[0] = CD3DX12_DESCRIPTOR_RANGE(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0);

    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(parameter[0], 1, &range[0], D3D12_SHADER_VISIBILITY_PIXEL);

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
        IID_PPV_ARGS(m_boardRootSig.GetAddressOf()));
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
    scDesc.Format = DEFAULT_BACK_BUFFER_FORMAT;
    scDesc.BufferCount = DEFAULT_BACK_BUFFER_COUNT;
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

void D3D12App::CreateFrameResources()
{
    m_frameResources.reserve(num_frame_resources);
    for (uint16_t i = 0; i < num_frame_resources; ++i)
        m_frameResources.emplace_back(m_device.Get());
    m_currentFrameResource = &m_frameResources[m_currentFrameResourceIndex];
}

bool D3D12App::CreateBackBufferView()
{
    D12Helper::CreateDescriptorHeap(m_device.Get(), m_bbRTVHeap, DEFAULT_BACK_BUFFER_COUNT, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    m_backBuffer.resize(DEFAULT_BACK_BUFFER_COUNT);
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.Format = DEFAULT_BACK_BUFFER_FORMAT;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    auto heap = m_bbRTVHeap->GetCPUDescriptorHandleForHeapStart();
    const auto increamentSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    for (int i = 0; i < DEFAULT_BACK_BUFFER_COUNT; ++i)
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

    CD3DX12_CLEAR_VALUE clearValue(
        DEFAULT_DEPTH_BUFFER_FORMAT                       // format
        ,1.0f                                       // depth
        ,0);                                        // stencil

    m_depthBuffer = D12Helper::CreateTexture2D(m_device.Get(),
        rtvDesc.Width, rtvDesc.Height, DEFAULT_DEPTH_BUFFER_FORMAT, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(), m_dsvHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = DEFAULT_DEPTH_BUFFER_FORMAT;
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

    CD3DX12_CLEAR_VALUE clearValue(
        DEFAULT_DEPTH_BUFFER_FORMAT                       // format
        , 1.0f                                       // depth
        , 0);                                        // stencil

    m_shadowDepthBuffer = D12Helper::CreateTexture2D(m_device.Get(),
        1024, 1024, DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(), m_shadowDSVHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    m_device->CreateDepthStencilView(
        m_shadowDepthBuffer.Get(),
        &dsvDesc,
        m_shadowDSVHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void D3D12App::CreateShadowRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[1] = {};
    D3D12_ROOT_PARAMETER rootParam[2] = {};

    // World pass constant
    CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(rootParam[0], 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    // Object constant
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        1,                                      // number of descriptors
        1);                                     // base shader register
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1], 1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);
   
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
        IID_PPV_ARGS(m_shadowRootSig.GetAddressOf()));
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
    psoDesc.DSVFormat = DEFAULT_DEPTH_BUFFER_FORMAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.NodeMask = 0;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // Output set up
    // Blend
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // Root Signature
    psoDesc.pRootSignature = m_shadowRootSig.Get();

    result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_shadowPipeline.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::CreateViewDepth()
{
    CreateViewDepthBuffer();
    CreateViewDepthRootSignature();
    CreateViewDepthPipelineState();
}

bool D3D12App::CreateViewDepthBuffer()
{
    HRESULT result = S_OK;
    auto rtvDesc = m_backBuffer[0]->GetDesc();

    CD3DX12_CLEAR_VALUE clearValue(
        DEFAULT_DEPTH_BUFFER_FORMAT                       // format
        , 1.0f                                       // depth
        , 0);                                        // stencil

    m_viewDepthBuffer = D12Helper::CreateTexture2D(m_device.Get(),
        rtvDesc.Width, rtvDesc.Height, DXGI_FORMAT_R32_TYPELESS, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue);

    D12Helper::CreateDescriptorHeap(m_device.Get(), m_viewDSVHeap, 1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Texture2D.MipSlice = 0; // number order[No] (NOT count of)
    dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
    m_device->CreateDepthStencilView(
        m_viewDepthBuffer.Get(),
        &dsvDesc,
        m_viewDSVHeap->GetCPUDescriptorHandleForHeapStart());

    return true;
}

void D3D12App::CreateViewDepthRootSignature()
{
    HRESULT result = S_OK;
    //Root Signature
    D3D12_ROOT_SIGNATURE_DESC rtSigDesc = {};
    rtSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Descriptor table
    D3D12_DESCRIPTOR_RANGE range[1] = {};
    D3D12_ROOT_PARAMETER rootParam[2] = {};

    // World pass constant
    CD3DX12_ROOT_PARAMETER::InitAsConstantBufferView(rootParam[0], 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    // Object constant
    range[0] = CD3DX12_DESCRIPTOR_RANGE(
        D3D12_DESCRIPTOR_RANGE_TYPE_CBV,        // range type
        1,                                      // number of descriptors
        1);                                     // base shader register
    CD3DX12_ROOT_PARAMETER::InitAsDescriptorTable(rootParam[1], 1, &range[0], D3D12_SHADER_VISIBILITY_VERTEX);

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
        IID_PPV_ARGS(m_viewDepthRootSig.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::CreateViewDepthPipelineState()
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
        L"Shader/VS.hlsl",                                  // path to shader file
        nullptr,                                            // define marcro object 
        D3D_COMPILE_STANDARD_FILE_INCLUDE,                  // include object
        "VS",                                               // entry name
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
    psoDesc.DSVFormat = DEFAULT_DEPTH_BUFFER_FORMAT;
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    psoDesc.NodeMask = 0;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    // Output set up
    // Blend
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    // Root Signature
    psoDesc.pRootSignature = m_viewDepthRootSig.Get();

    result = m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_viewDepthPipeline.GetAddressOf()));
    assert(SUCCEEDED(result));
}

void D3D12App::RenderToShadowDepthBuffer()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeap(m_shadowDSVHeap->GetCPUDescriptorHandleForHeapStart());
    m_cmdList->OMSetRenderTargets(0, nullptr, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    m_cmdList->SetPipelineState(m_shadowPipeline.Get());
    m_cmdList->SetGraphicsRootSignature(m_shadowRootSig.Get());

    CD3DX12_VIEWPORT vp(m_shadowDepthBuffer.Get());
    m_cmdList->RSSetViewports(1, &vp);

    auto desc = m_shadowDepthBuffer->GetDesc();
    CD3DX12_RECT rc(0, 0, desc.Width, desc.Height);
    m_cmdList->RSSetScissorRects(1, &rc);

    m_pmdManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
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

void D3D12App::RenderToViewDepthBuffer()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeap(m_viewDSVHeap->GetCPUDescriptorHandleForHeapStart());
    m_cmdList->OMSetRenderTargets(0, nullptr, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    m_cmdList->SetPipelineState(m_viewDepthPipeline.Get());
    m_cmdList->SetGraphicsRootSignature(m_viewDepthRootSig.Get());

    CD3DX12_VIEWPORT vp(m_viewDepthBuffer.Get());
    m_cmdList->RSSetViewports(1, &vp);

    auto desc = m_viewDepthBuffer->GetDesc();
    CD3DX12_RECT rc(0, 0, desc.Width, desc.Height);
    m_cmdList->RSSetScissorRects(1, &rc);

    m_pmdManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_pmdManager->RenderDepth(m_cmdList.Get());

    // After draw to shadow buffer, change its state from DSV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_viewDepthBuffer.Get(),                       // resource buffer
            D3D12_RESOURCE_STATE_DEPTH_WRITE,               // state before
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);    // state after
    m_cmdList->ResourceBarrier(1, &barrier);
}

void D3D12App::WaitForGPU()
{
    // Check GPU current fence value 
    // If GPU current fence value haven't reached target Fence Value
    // Tell CPU to wait until GPU reach current fence
    if (m_fence->GetCompletedValue() < m_targetFenceValue)
    {
        auto fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fence->SetEventOnCompletion(m_targetFenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    };
}

void D3D12App::UpdateFence()
{
    // Set current GPU target fence value to next process frame
    m_currentFrameResource->FenceValue = ++m_targetFenceValue;

    // use Signal of Command Que to tell GPU to set current GPU fence value
    // to next process frame, after finished last Execution Draw Call
    m_cmdQue->Signal(m_fence.Get(), m_targetFenceValue);
}

void D3D12App::CreatePostEffect()
{
    CreateBoardPolygonVertices();

    CreateRenderTargetTexture();
    CreateNormalMapTexture();
    CreateBoardShadowDepthView();
    CreateBoardViewDepthView();

    CreateBoardRootSignature();
    CreateBoardPipeline();
}

D3D12App::D3D12App()
{
}

D3D12App::~D3D12App()
{
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
    CreateFrameResources();

    m_cmdAlloc->Reset();
    m_cmdList->Reset(m_cmdAlloc.Get(), nullptr);

    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.ReleaseAndGetAddressOf()));
    m_targetFenceValue = m_fence->GetCompletedValue();
    
    CreateSwapChain(hwnd);
    CreateBackBufferView();
    CreateDepthBuffer();  
    CreateWorldPassConstant();
    CreateMaterials();
    CreateShadowMapping();
    CreateViewDepth();
    CreatePostEffect();
    CreateDefaultTexture();
    CreatePMDModel();
    CreatePrimitive();

    m_cmdList->Close();
    m_cmdQue->ExecuteCommandLists(1, reinterpret_cast<ID3D12CommandList*const*>(m_cmdList.GetAddressOf()));
    UpdateFence();
    WaitForGPU();

    EffekseerInit();

    m_updateBuffers.Clear();
    m_pmdManager->ClearSubresources();
    m_primitiveManager->ClearSubresources();

    return true;
}

bool D3D12App::CreateWorldPassConstant()
{
    HRESULT result = S_OK;

    m_worldPCBuffer.Create(m_device.Get(), num_frame_resources, true);

    auto& app = Application::Instance();

    m_camera.SetCameraPosition(10.0f, 10.0f, 10.0f);
    m_camera.SetFOVAngle(XM_PIDIV2);
    m_camera.SetTargetPosition(0.0f, 0.0f, 0.0f);
    m_camera.SetAspectRatio(app.GetAspectRatio());
    m_camera.SetViewFrustum(0.1f, 500.0f);
    m_camera.Init();

    auto gpuAddress = m_worldPCBuffer.GetGPUVirtualAddress();
    const auto stride_bytes = m_worldPCBuffer.ElementSize();
    for (uint16_t i = 0; i < num_frame_resources; ++i)
    {
        auto mappedData = m_worldPCBuffer.GetHandleMappedData(i);
        mappedData->ViewPos = m_camera.GetCameraPosition();
        mappedData->ViewProj = m_camera.GetViewProjectionMatrix();

        XMVECTOR lightPos = { 10,30,0,1 };
       
        mappedData->Lights[0] = Light();
        //mappedData->Lights[1] = Light();
        //mappedData->Lights[2] = Light();
        //mappedData->Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
        //mappedData->Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
        //mappedData->Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
        //mappedData->Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
        //mappedData->Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
        //mappedData->Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

        //auto lightDirection = XMLoadFloat3(&mappedData->Lights[0].Direction);
        //auto lightViewProj = XMMatrixLookToRH(lightPos, lightDirection, { 0,1,0,0 }) *
        //    XMMatrixOrthographicRH(200.0f, 200.0f, 1.0f, 500.0f);

        XMStoreFloat3(&mappedData->Lights[0].Position, lightPos);
        mappedData->Lights[0].Strength = { 0.6f, 0.6f, 0.6f };

        XMVECTOR targetPos = { 0.0f,0.0f,0.0f,1.0f };
        auto lightDirection = XMVectorSubtract(targetPos, lightPos);
        lightDirection = XMVector4Normalize(lightDirection);
        XMStoreFloat3(&mappedData->Lights[0].Direction, lightDirection);

        auto lightViewProj = XMMatrixLookAtRH(lightPos, targetPos, { 0.0f,1.0f,0.0f,0.0f }) *
            XMMatrixPerspectiveFovRH(XM_PIDIV2, 1.0f, 1.0f, 500.0f);

        XMStoreFloat4x4(&mappedData->Lights[0].ProjectMatrix, lightViewProj);

        gpuAddress += stride_bytes;
    }

    return true;
}

void D3D12App::CreateMaterials()
{
    m_materialCB.Create(m_device.Get(), 3, true);

    uint16_t index = 0;
    m_materialIndices["brick0"] = index;
    auto handledMappedData = m_materialCB.GetHandleMappedData(index);
    handledMappedData->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
    handledMappedData->FresnelF0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    handledMappedData->Roughness = 0.1f;
    ++index;

    m_materialIndices["stone0"] = index;
    handledMappedData = m_materialCB.GetHandleMappedData(index);
    handledMappedData->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
    handledMappedData->FresnelF0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    handledMappedData->Roughness = 0.3f;
    ++index;

    m_materialIndices["tile0"] = index;
    handledMappedData = m_materialCB.GetHandleMappedData(index);
    handledMappedData->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
    handledMappedData->FresnelF0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    handledMappedData->Roughness = 0.2f;
}                                  

void D3D12App::CreatePMDModel()
{
    m_pmdManager = std::make_unique<PMDManager>();

    m_pmdManager->SetDevice(m_device.Get());
    m_pmdManager->SetDefaultBuffer(m_whiteTexture.Get(), m_blackTexture.Get(), m_gradTexture.Get());
    m_pmdManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress());
    m_pmdManager->SetWorldShadowMap(m_shadowDepthBuffer.Get());
    m_pmdManager->CreateModel("Hibiki", model2_path);
    m_pmdManager->CreateModel("Miku", model1_path);
    m_pmdManager->CreateModel("Haku", model_path);
    m_pmdManager->CreateAnimation("Dancing1", motion1_path);
    m_pmdManager->CreateAnimation("Dancing2", motion2_path);

    assert(m_pmdManager->Init(m_cmdList.Get()));
    assert(m_pmdManager->Play("Miku", "Dancing1"));
    assert(m_pmdManager->Play("Hibiki", "Dancing1"));

    return;
}

void D3D12App::CreatePrimitive()
{
    m_primitiveManager = std::make_unique<PrimitiveManager>();

    auto brickGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["brick0"]);
    auto stoneGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["stone0"]);
    auto tileGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["tile0"]);

    m_primitiveManager->SetDevice(m_device.Get());
    m_primitiveManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress());
    m_primitiveManager->SetWorldShadowMap(m_shadowDepthBuffer.Get());
    //m_primitiveManager->SetViewDepth(m_viewDepthBuffer.Get());
    m_primitiveManager->Create("grid", GeometryGenerator::CreateGrid(200.0f, 100.0f, 30, 40), tileGpuAdress);
    m_primitiveManager->Create("sphere", GeometryGenerator::CreateSphere(5.0f, 20, 20) , stoneGpuAdress);
    m_primitiveManager->Create("sphere1", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere2", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere3", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere4", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere5", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere6", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere7", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere8", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere9", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere10", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere11", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere12", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("sphere13", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress);
    m_primitiveManager->Create("cylinder", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1) , brickGpuAdress);
    m_primitiveManager->Create("cylinder1", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder2", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder3", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder4", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder5", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder6", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder7", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder8", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder9", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder10", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder11", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder12", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    m_primitiveManager->Create("cylinder13", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress);
    assert(m_primitiveManager->Init(m_cmdList.Get()));

    float startX = 100.0f;
    float startZ = 50.0f;
    const float offsetX = 200.0f / 6;
    const float offsetZ = 100.0f / 6;
    m_primitiveManager->Move("sphere", startX, 25, startZ);
    m_primitiveManager->Move("cylinder", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere1", startX, 25, startZ);
    m_primitiveManager->Move("cylinder1", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere2", startX, 25, startZ);
    m_primitiveManager->Move("cylinder2", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere3", startX, 25, startZ);
    m_primitiveManager->Move("cylinder3", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere4", startX, 25, startZ);
    m_primitiveManager->Move("cylinder4", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere5", startX, 25, startZ);
    m_primitiveManager->Move("cylinder5", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere6", startX, 25, startZ);
    m_primitiveManager->Move("cylinder6", startX, 10, startZ);

    startX = 100.0f;
    startZ = -50.0f;
    m_primitiveManager->Move("sphere7", startX, 25, startZ);
    m_primitiveManager->Move("cylinder7", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere8", startX, 25, startZ);
    m_primitiveManager->Move("cylinder8", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere9", startX, 25, startZ);
    m_primitiveManager->Move("cylinder9", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere10", startX, 25, startZ);
    m_primitiveManager->Move("cylinder10", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere11", startX, 25, startZ);
    m_primitiveManager->Move("cylinder11", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere12", startX, 25, startZ);
    m_primitiveManager->Move("cylinder12", startX, 10, startZ);
    startX -= offsetX;
    m_primitiveManager->Move("sphere13", startX, 25, startZ);
    m_primitiveManager->Move("cylinder13", startX, 10, startZ);
}

bool D3D12App::ProcessMessage()
{
    m_keyboard.ProcessMessage();
    m_mouse.ProcessMessage();
    return true;
}

bool D3D12App::Update(const float& deltaTime)
{
    UpdateCamera(deltaTime);
    EffekseerUpdate(deltaTime);

    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % num_frame_resources;
    m_currentFrameResource = &m_frameResources[m_currentFrameResourceIndex];

    // Check if current GPU fence value is processing at current frame resource
    // If current GPU fence value < current frame resource value
    // -> GPU is processing at current frame resource
    if (m_currentFrameResource->FenceValue != 0 && m_fence->GetCompletedValue() < m_currentFrameResource->FenceValue)
    {
        auto fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fence->SetEventOnCompletion(m_targetFenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    }

    if (m_keyboard.IsPressed('R'))
        m_pmdManager->RotateY("Miku", 1.0f*deltaTime);

    if (m_keyboard.IsPressed(VK_LEFT))
        m_pmdManager->Move("Hibiki", 10.0f * deltaTime, 0.0f, 0.0f);
    if (m_keyboard.IsPressed(VK_RIGHT))
        m_pmdManager->Move("Haku", -10.0f * deltaTime, 0.0f, 0.0f);

    UpdateWorldPassConstant();
    m_pmdManager->Update(deltaTime);
    
    g_scalar = g_scalar > 5 ? 0.1 : g_scalar;

    return true;
}

void D3D12App::SetResourceStateForNextFrame()
{
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtTexture.Get(),
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
    auto& cmdAlloc = m_currentFrameResource->CmdAlloc;
    cmdAlloc->Reset();
    m_cmdList->Reset(cmdAlloc.Get(), nullptr);

    RenderToShadowDepthBuffer();
    RenderToViewDepthBuffer();
    RenderToRenderTargetTexture();
    RenderToBackBuffer();
    SetResourceStateForNextFrame();

    m_cmdList->Close();
    m_cmdQue->ExecuteCommandLists(1, (ID3D12CommandList* const*)m_cmdList.GetAddressOf());
    // screen flip
    m_swapchain->Present(0, 0);

    SetBackBufferIndexForNextFrame();
    UpdateFence();

    return true;
}

void D3D12App::Terminate()
{
    EffekseerTerminate();
    m_currentFrameResource = nullptr;
}

void D3D12App::ClearKeyState()
{
    m_keyboard.Reset();
}

void D3D12App::OnWindowsKeyboardMessage(uint32_t msg, WPARAM keycode, LPARAM lparam)
{
    m_keyboard.OnWindowsMessage(msg, keycode, lparam);
}

void D3D12App::OnWindowsMouseMessage(uint32_t msg, WPARAM keycode, LPARAM lparam)
{
    m_mouse.OnWindowsMessage(msg, keycode, lparam);
}
