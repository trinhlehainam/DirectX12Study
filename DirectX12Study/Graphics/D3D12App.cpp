#include "D3D12App.h"

#include <iostream>
#include <cassert>
#include <algorithm>

#include <d3dcompiler.h>
#include <DirectXTex.h>
#include <DirectXColors.h>
#include <dxgidebug.h>

#include "RootSignature.h"
#include "GraphicsPSO.h"
#include "TextureManager.h"
#include "PipelineManager.h"
#include "BlurFilter.h"
#include "SpriteManager.h"
#include "../Application.h"
#include "../Loader/BmpLoader.h"
#include "../PMDModel/PMDManager.h"
#include "../Geometry/GeometryGenerator.h"
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
        m_updateBuffers.GetSubresource(whiteTexStr),1);

    std::vector<Color> blackTex(4 * 4);
    std::fill(blackTex.begin(), blackTex.end(), Color(0x00, 0x00, 0x00, 0xff));
    const std::string blackTexStr = "black texture";
    m_updateBuffers.Add(blackTexStr, whiteTex.data(), sizeof(whiteTex[0]) * 4, whiteTex.size() * 4);
    D12Helper::UpdateDataToTextureBuffer(m_device.Get(), m_cmdList.Get(), m_whiteTexture,
        m_updateBuffers.GetBuffer(blackTexStr),
        m_updateBuffers.GetSubresource(blackTexStr),1);
    
    std::vector<Color> gradTex(4 * 4);
    gradTex.resize(256);
    for (size_t i = 0; i < 256; ++i)
        std::fill_n(&gradTex[i], 1, Color(255 - i, 255 - i, 255 - i, 0xff));
    const std::string gradTexStr = "gradiation texture";
    m_updateBuffers.Add(gradTexStr, whiteTex.data(), sizeof(whiteTex[0]) * 4, whiteTex.size() * 4);
    D12Helper::UpdateDataToTextureBuffer(m_device.Get(), m_cmdList.Get(), m_whiteTexture,
        m_updateBuffers.GetBuffer(gradTexStr),
        m_updateBuffers.GetSubresource(gradTexStr),1);
    
}

void D3D12App::CreateRenderTargetTexture()
{
    HRESULT result = S_OK;
    auto rsDesc = m_backBuffer[0]->GetDesc();
    auto worldPCB = m_worldPCBuffer.GetHandleMappedData(m_currentFrameResourceIndex);
    float fogColor[] = { worldPCB->FogColor.x,worldPCB->FogColor.y,worldPCB->FogColor.z,worldPCB->FogColor.w };
    auto clearValue = CD3DX12_CLEAR_VALUE(rsDesc.Format, fogColor);
    m_rtTex = D12Helper::CreateTexture2D(m_device.Get(),
        rsDesc.Width, rsDesc.Height, rsDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    float color[] = { 0.f,0.0f,0.0f,1.0f };
    clearValue = CD3DX12_CLEAR_VALUE(rsDesc.Format, color);
    m_rtNormalTex = D12Helper::CreateTexture2D(m_device.Get(),
        rsDesc.Width, rsDesc.Height, rsDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    m_rtBrightTex = D12Helper::CreateTexture2D(m_device.Get(),
        rsDesc.Width, rsDesc.Height, rsDesc.Format, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
        D3D12_HEAP_TYPE_DEFAULT,
        D3D12_RESOURCE_STATE_RENDER_TARGET, &clearValue);

    // RTT for back buffer
    // RTT for normal texture
    // RTT for bright color ( that color > 1.0f )
    const size_t num_rtx = 3;
    D12Helper::CreateDescriptorHeap(m_device.Get(),m_rtvHeap, num_rtx, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    // Main render target
    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = rsDesc.Format;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    // Scene texture
    m_device->CreateRenderTargetView(m_rtTex.Get(), &rtvDesc, rtvHeapHandle);

    // Normal texture
    rtvHeapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    m_device->CreateRenderTargetView(m_rtNormalTex.Get(), &rtvDesc, rtvHeapHandle);

    // Bright texture
    rtvHeapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    m_device->CreateRenderTargetView(m_rtBrightTex.Get(), &rtvDesc, rtvHeapHandle);

    //
    // SRV 6 resources
    //
    // Render target texture
    // Render target normal texture
    // Render target bright texture
    // Normal map texture
    // Shadow depth texture
    // View Depth Texture
    D12Helper::CreateDescriptorHeap(m_device.Get(),m_boardSRVHeap, 6, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);

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
    m_device->CreateShaderResourceView(m_rtTex.Get(), &srvDesc, srvHeapHandle);

    // 2nd SRV
    // Render target normal texture
    const int rt_normal_index = 1;
    srvHeapHandle.Offset(rt_normal_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    m_device->CreateShaderResourceView(m_rtNormalTex.Get(), &srvDesc, srvHeapHandle);

    // 3rd SRV
    // Render target bright texture
    srvHeapHandle.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
    m_device->CreateShaderResourceView(m_rtBrightTex.Get(), &srvDesc, srvHeapHandle);
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

    // 5th SRV
    const int shadow_depth_index = 4;
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

    // 6th SRV
    const int view_depth_index = 5;
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_boardSRVHeap->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(view_depth_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateShaderResourceView(m_viewDepthBuffer.Get(), &srvDesc, heapHandle);
}

void D3D12App::RenderToRenderTargetTextures()
{
    auto worldPCB = m_worldPCBuffer.GetHandleMappedData(m_currentFrameResourceIndex);

    auto wsize = Application::Instance().GetWindowSize();
    CD3DX12_VIEWPORT vp(m_backBuffer[m_currentBackBuffer].Get());
    m_cmdList->RSSetViewports(1, &vp);

    CD3DX12_RECT rc(0, 0, wsize.width, wsize.height);
    m_cmdList->RSSetScissorRects(1, &rc);

    float rtTexDefaultColor[4] = { 0.0f,0.0f,0.0f,1.0f };
    float fogColor[4] = { worldPCB->FogColor.x,worldPCB->FogColor.y,worldPCB->FogColor.z,worldPCB->FogColor.w };
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtTexHeap(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtNormalTexHeap(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtNormalTexHeap.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtBrightTexHeap(rtNormalTexHeap);
    rtBrightTexHeap.Offset(1, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtHeapHandle[] = { rtTexHeap , rtNormalTexHeap, rtBrightTexHeap };
    auto dsvHeap = m_viewDSVHeap->GetCPUDescriptorHandleForHeapStart();
    m_cmdList->OMSetRenderTargets(_countof(rtHeapHandle), rtHeapHandle, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    m_cmdList->ClearRenderTargetView(rtTexHeap, fogColor, 0, nullptr);
    m_cmdList->ClearRenderTargetView(rtNormalTexHeap, rtTexDefaultColor, 0, nullptr);
    m_cmdList->ClearRenderTargetView(rtBrightTexHeap, rtTexDefaultColor, 0, nullptr);

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("pmd"));
    m_cmdList->SetGraphicsRootSignature(m_psoMng->GetRootSignature("pmd"));
    m_pmdManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_pmdManager->Render(m_cmdList.Get());

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("primitive"));
    m_cmdList->SetGraphicsRootSignature(m_psoMng->GetRootSignature("primitive"));
    m_primitiveManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_primitiveManager->Render(m_cmdList.Get());

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("tree"));
    m_cmdList->SetGraphicsRootSignature(m_psoMng->GetRootSignature("tree"));
    m_spriteMng->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_spriteMng->Render(m_cmdList.Get());

    EffekseerRender();

    D12Helper::TransitionResourceState(m_cmdList.Get(), m_rtTex.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);

    m_blurFilter->Blur(m_cmdList.Get(), m_rtTex.Get(), 8);

    D12Helper::TransitionResourceState(m_cmdList.Get(), m_rtTex.Get(),
        D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Set resource state of postEffectTexture from RTV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtTex.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtNormalTex.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtBrightTex.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    m_cmdList->ResourceBarrier(1, &barrier);

    // After draw to shadow buffer, change its state from DSV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_viewDepthBuffer.Get(),                       // resource buffer
            D3D12_RESOURCE_STATE_DEPTH_WRITE,               // state before
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);    // state after
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

    float bbDefaultColor[] = { 0.0f,0.0f,0.0f,1.0f };
    m_cmdList->OMSetRenderTargets(1, &bbRTVHeap, false, nullptr);
    m_cmdList->ClearRenderTargetView(bbRTVHeap, bbDefaultColor, 0, nullptr);

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("board"));
    m_cmdList->SetGraphicsRootSignature(m_psoMng->GetRootSignature("board"));

    m_cmdList->SetDescriptorHeaps(1, m_boardSRVHeap.GetAddressOf());
    CD3DX12_GPU_DESCRIPTOR_HANDLE shaderHeap(m_boardSRVHeap->GetGPUDescriptorHandleForHeapStart());
    m_cmdList->SetGraphicsRootDescriptorTable(0, shaderHeap);

    m_cmdList->IASetVertexBuffers(0, 1, &m_boardVBV);
    m_cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_cmdList->DrawInstanced(4, 1, 0, 0);

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
        DXGI_FORMAT_D32_FLOAT, false, 8000);

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

bool D3D12App::CreateImGui(const HWND& hwnd)
{
    CreateImGuiDescriptorHeap();
    if (!ImGui::CreateContext())
        return false;
    if (!ImGui_ImplWin32_Init(hwnd))
        return false;
    if (!ImGui_ImplDX12_Init(m_device.Get(), 1, DEFAULT_BACK_BUFFER_FORMAT, m_imguiHeap.Get(),
        m_imguiHeap->GetCPUDescriptorHandleForHeapStart(),
        m_imguiHeap->GetGPUDescriptorHandleForHeapStart()))
        return false;

    return true;
}

void D3D12App::CreateImGuiDescriptorHeap()
{
    D12Helper::CreateDescriptorHeap(m_device.Get(), m_imguiHeap, 1, 
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true);
}

void D3D12App::UpdateImGui(float deltaTime)
{
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Hello World!");
    static auto wsize = ImGui::GetWindowSize();
    ImGui::SetWindowSize(wsize);
    wsize = ImGui::GetWindowSize();
    static float col[3] = {0.0f,0.0f,0.0f};
    ImGui::ColorPicker3("Color3", col);
    static bool flag = false;
    ImGui::Checkbox("Sample Box", &flag);
    ImGui::End();
}

void D3D12App::RenderImGui()
{

    ImGui::Render();

    m_cmdList->SetDescriptorHeaps(1, m_imguiHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_cmdList.Get());
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

    // 4rd SRV
    const int normal_texture_index = 3;
    CD3DX12_CPU_DESCRIPTOR_HANDLE heapHandle(m_boardSRVHeap->GetCPUDescriptorHandleForHeapStart());
    heapHandle.Offset(normal_texture_index, m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));

    m_device->CreateShaderResourceView(m_normalMapTex.Get(), &srvDesc, heapHandle);
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
    ComPtr<IDXGISwapChain1> swapChain1;
    auto result = m_dxgi->CreateSwapChainForHwnd(
        m_cmdQue.Get(),
        hwnd,
        &scDesc,
        nullptr,
        nullptr,
        swapChain1.GetAddressOf());
    assert(SUCCEEDED(result));
    swapChain1->QueryInterface(IID_PPV_ARGS(&m_swapchain));
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

void D3D12App::RenderToShadowDepthBuffer()
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeap(m_shadowDSVHeap->GetCPUDescriptorHandleForHeapStart());
    m_cmdList->OMSetRenderTargets(0, nullptr, false, &dsvHeap);
    m_cmdList->ClearDepthStencilView(dsvHeap, D3D12_CLEAR_FLAG_DEPTH, 1.f, 0, 0, nullptr);

    CD3DX12_VIEWPORT vp(m_shadowDepthBuffer.Get());
    m_cmdList->RSSetViewports(1, &vp);

    auto desc = m_shadowDepthBuffer->GetDesc();
    CD3DX12_RECT rc(0, 0, desc.Width, desc.Height);
    m_cmdList->RSSetScissorRects(1, &rc);

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("primitiveShadow"));
    m_cmdList->SetGraphicsRootSignature(m_psoMng->GetRootSignature("shadow"));
    m_primitiveManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_primitiveManager->RenderDepth(m_cmdList.Get());

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("shadow"));
    m_pmdManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_pmdManager->RenderDepth(m_cmdList.Get());

    m_cmdList->SetPipelineState(m_psoMng->GetPSO("treeShadow"));

    // After draw to shadow buffer, change its state from DSV -> SRV
    // -> Ready to be used as SRV when Render to Back Buffer
    D3D12_RESOURCE_BARRIER barrier =
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_shadowDepthBuffer.Get(),                       // resource buffer
            D3D12_RESOURCE_STATE_DEPTH_WRITE,               // state before
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);    // state after
    m_cmdList->ResourceBarrier(1, &barrier);
}

void D3D12App::WaitForGPU()
{
    // Check GPU current fence value 
    // If GPU current fence value haven't reached target Fence Value
    // Tell CPU to wait until GPU reach current fence
    auto fenceValue = m_fence->GetCompletedValue();
    if (fenceValue < m_targetFenceValue)
    {
        auto fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fence->SetEventOnCompletion(m_targetFenceValue, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);
    };
}

void D3D12App::CreateTextureManager()
{
    m_texMng = std::make_unique<TextureManager>();
    m_texMng->SetDevice(m_device.Get());

    m_texMng->Create(m_cmdList.Get(), "crate", L"resource/image/Textures/WoodCrate02.dds");
    m_texMng->Create(m_cmdList.Get(), "wire-fence", L"resource/image/Textures/WireFence.dds");
    m_texMng->Create(m_cmdList.Get(), "tile", L"resource/image/Textures/tile.dds");
    m_texMng->Create(m_cmdList.Get(), "brick", L"resource/image/Textures/bricks.dds");
    m_texMng->Create(m_cmdList.Get(), "stone", L"resource/image/Textures/stone.dds");
    m_texMng->Create(m_cmdList.Get(), "tree0", L"resource/image/Textures/tree01S.dds");
    m_texMng->Create(m_cmdList.Get(), "tree1", L"resource/image/Textures/tree02S.dds");
    m_texMng->Create(m_cmdList.Get(), "tree2", L"resource/image/Textures/tree35S.dds");
}

void D3D12App::CreatePSOManager()
{
    m_psoMng = std::make_unique<PipelineManager>();
}

void D3D12App::CreatePipelines()
{
    CreateInputLayouts();
    CreateRootSignatures();
    CreatePSOs();
}

void D3D12App::CreateInputLayouts()
{
    D3D12_INPUT_ELEMENT_DESC boardLayout[] = {
    {
    "POSITION",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    0,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0}
    };
    m_psoMng->CreateInputLayout("board", _countof(boardLayout), boardLayout);

    D3D12_INPUT_ELEMENT_DESC pmdLayout[] = {
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
    m_psoMng->CreateInputLayout("pmd", _countof(pmdLayout), pmdLayout);

    D3D12_INPUT_ELEMENT_DESC primitiveLayout[] = {
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
    m_psoMng->CreateInputLayout("primitive", _countof(primitiveLayout), primitiveLayout);

    D3D12_INPUT_ELEMENT_DESC treeLayout[] = {
    {
    "POSITION",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32B32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    0,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0},
    {
    "SIZE",                                   //semantic
    0,                                            //semantic index(配列の場合に配列番号を入れる)
    DXGI_FORMAT_R32G32_FLOAT,                  // float3 -> [3D array] R32G32B32
    0,                                            //スロット番号（頂点データが入ってくる入口地番号）
    D3D12_APPEND_ALIGNED_ELEMENT,                                            //このデータが何バイト目から始まるのか
    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,   //頂点ごとのデータ
    0
    }
    };
    m_psoMng->CreateInputLayout("tree", _countof(treeLayout), treeLayout);
}

void D3D12App::CreateRootSignatures()
{
    RootSignature rootSig;
    // Main Render target texture 
    // Normal render target texture
    // Normal texture
    // Shadow depth texture
    // View depth texture
    rootSig.AddRootParameterAsDescriptorTable(0, 6, 0);
    rootSig.AddStaticSampler(RootSignature::LINEAR_WRAP);
    rootSig.AddStaticSampler(RootSignature::LINEAR_BORDER);
    rootSig.Create(m_device.Get());
    m_psoMng->CreateRootSignature("board", rootSig.Get());

    //
    // Shadow
    //
    rootSig.Reset();
    // World Constant
    rootSig.AddRootParameterAsDescriptor(RootSignature::CBV);
    // Object Constant
    rootSig.AddRootParameterAsDescriptorTable(1, 0, 0);
    rootSig.Create(m_device.Get());
    m_psoMng->CreateRootSignature("shadow", rootSig.Get());

    //
    // PMD
    //
    rootSig.Reset();
    // World pass constant
    rootSig.AddRootParameterAsDescriptor(RootSignature::CBV);
    // Depth
    rootSig.AddRootParameterAsDescriptorTable(0, 2, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    // Object Constant
    rootSig.AddRootParameterAsDescriptorTable(1, 0, 0);
    // Material Constant
    rootSig.AddRootParameterAsDescriptorTable(1, 4, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootSig.AddStaticSampler(RootSignature::LINEAR_WRAP);
    rootSig.AddStaticSampler(RootSignature::LINEAR_CLAMP);
    rootSig.AddStaticSampler(RootSignature::COMPARISION_LINEAR_WRAP);
    rootSig.Create(m_device.Get());
    m_psoMng->CreateRootSignature("pmd", rootSig.Get());

    //
    // Primitive
    //
    rootSig.Reset();
    rootSig.AddRootParameterAsDescriptor(RootSignature::CBV);
    // shadow depth buffer
    rootSig.AddRootParameterAsDescriptorTable(0, 1, 0);
    // Object constant
    rootSig.AddRootParameterAsDescriptorTable(1, 0, 0);
    // Texture
    rootSig.AddRootParameterAsDescriptorTable(0, 1, 0);
    // Material constant
    rootSig.AddRootParameterAsDescriptor(RootSignature::CBV);
    rootSig.AddStaticSampler(RootSignature::LINEAR_WRAP);
    rootSig.AddStaticSampler(RootSignature::LINEAR_BORDER);
    rootSig.Create(m_device.Get());
    m_psoMng->CreateRootSignature("primitive", rootSig.Get());

    //
    // Tree BillBoard
    //
    rootSig.Reset();
    // World Pass Constant
    rootSig.AddRootParameterAsDescriptor(RootSignature::CBV);
    // Object Constant
    // Texture
    rootSig.AddRootParameterAsDescriptorTable(1, 1, 0);
    rootSig.AddStaticSampler(RootSignature::LINEAR_WRAP);
    rootSig.Create(m_device.Get());
    m_psoMng->CreateRootSignature("tree", rootSig.Get());
}

void D3D12App::CreatePSOs()
{
    auto rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;

    auto depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    auto blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;
    ComPtr<ID3DBlob> gsBlob;

    GraphicsPSO pso;

    pso.SetInputLayout(m_psoMng->GetInputLayout("board"));
    pso.SetPrimitiveTopology();
    pso.SetRenderTargetFormat();
    pso.SetSampleMask();
    pso.SetRasterizerState(rasterizerDesc);
    pso.SetBlendState(blendDesc);
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/boardVS.hlsl", "boardVS", "vs_5_1");
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    psBlob = D12Helper::CompileShaderFromFile(L"Shader/boardPS.hlsl", "boardPS", "ps_5_1");
    pso.SetPixelShader(CD3DX12_SHADER_BYTECODE(psBlob.Get()));
    pso.SetRootSignature(m_psoMng->GetRootSignature("board"));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("board", pso.Get());

    //
    // Shadow
    //
    pso.Reset();
    pso.SetInputLayout(m_psoMng->GetInputLayout("pmd"));
    pso.SetPrimitiveTopology();
    pso.SetSampleMask();
    pso.SetDepthStencilFormat();
    pso.SetDepthStencilState(depthStencilDesc);
    pso.SetRasterizerState(rasterizerDesc);
    D3D_SHADER_MACRO defines[] = { "SHADOW_PIPELINE", "1", nullptr, nullptr };
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/vs.hlsl", "VS", "vs_5_1", defines);
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    pso.SetRootSignature(m_psoMng->GetRootSignature("shadow"));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("shadow", pso.Get());

    //
    // Primitive Shadow
    //
    pso.SetInputLayout(m_psoMng->GetInputLayout("primitive"));
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/primitiveVS.hlsl", "primitiveVS", "vs_5_1", defines);
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("primitiveShadow", pso.Get());

    //
    // Billboard Sprite Shadow
    //
    pso.SetInputLayout(m_psoMng->GetInputLayout("tree"));
    pso.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/TreeBillboardVS.hlsl", "TreeBillboardVS", "vs_5_1");
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    gsBlob = D12Helper::CompileShaderFromFile(L"Shader/TreeBillboardGS.hlsl", "TreeBillboardGS", "gs_5_1", defines);
    pso.SetGeometryShader(CD3DX12_SHADER_BYTECODE(gsBlob.Get()));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("treeShadow", pso.Get());

    //
    // PMD
    //
    pso.Reset();
    pso.SetInputLayout(m_psoMng->GetInputLayout("pmd"));
    pso.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
    pso.SetSampleMask();
    pso.SetRenderTargetFormats(3);
    pso.SetRasterizerState(rasterizerDesc);
    pso.SetDepthStencilState(depthStencilDesc);
    pso.SetBlendState(blendDesc);
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/vs.hlsl", "VS", "vs_5_1");
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    const D3D_SHADER_MACRO pmdDefines[] =
    {
        "FOG" , "1",
        nullptr, nullptr
    };
    psBlob = D12Helper::CompileShaderFromFile(L"Shader/ps.hlsl", "PS", "ps_5_1", pmdDefines);
    pso.SetPixelShader(CD3DX12_SHADER_BYTECODE(psBlob.Get()));
    pso.SetRootSignature(m_psoMng->GetRootSignature("pmd"));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("pmd", pso.Get());

    //
    // Primitive
    //
    pso.Reset();
    pso.SetInputLayout(m_psoMng->GetInputLayout("primitive"));
    pso.SetPrimitiveTopology();
    pso.SetSampleMask();
    pso.SetRenderTargetFormats(3);
    pso.SetRasterizerState(rasterizerDesc);
    pso.SetDepthStencilState(CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT));
    D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
    rtBlendDesc.BlendEnable = true;
    rtBlendDesc.LogicOpEnable = false;
    rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[0] = rtBlendDesc;
    pso.SetBlendState(blendDesc);
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/primitiveVS.hlsl", "primitiveVS", "vs_5_1");
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    const D3D_SHADER_MACRO primitiveDefines[] =
    { "ALPHA_TEST", "1",
        "FOG", "1",
        nullptr, nullptr };
    psBlob = D12Helper::CompileShaderFromFile(L"Shader/primitivePS.hlsl", "primitivePS", "ps_5_1", primitiveDefines);
    pso.SetPixelShader(CD3DX12_SHADER_BYTECODE(psBlob.Get()));
    pso.SetRootSignature(m_psoMng->GetRootSignature("primitive"));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("primitive", pso.Get());

    //
    // Tree Billboard
    //
    pso.Reset();
    pso.SetInputLayout(m_psoMng->GetInputLayout("tree"));
    pso.SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
    pso.SetSampleMask();
    pso.SetRenderTargetFormats(3);
    pso.SetRasterizerState(rasterizerDesc);
    pso.SetDepthStencilState(depthStencilDesc);
    pso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
    vsBlob = D12Helper::CompileShaderFromFile(L"Shader/TreeBillboardVS.hlsl", "TreeBillboardVS", "vs_5_1");
    pso.SetVertexShader(CD3DX12_SHADER_BYTECODE(vsBlob.Get()));
    gsBlob = D12Helper::CompileShaderFromFile(L"Shader/TreeBillboardGS.hlsl", "TreeBillboardGS", "gs_5_1");
    pso.SetGeometryShader(CD3DX12_SHADER_BYTECODE(gsBlob.Get()));
    psBlob = D12Helper::CompileShaderFromFile(L"Shader/TreeBillboardPS.hlsl", "TreeBillboardPS", "ps_5_1");
    pso.SetPixelShader(CD3DX12_SHADER_BYTECODE(psBlob.Get()));
    pso.SetRootSignature(m_psoMng->GetRootSignature("tree"));
    pso.Create(m_device.Get());
    m_psoMng->CreatePSO("tree", pso.Get());
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
}

D3D12App::D3D12App()
{
}

D3D12App::~D3D12App()
{    
    
#ifdef _DEBUG
    {
        ComPtr<IDXGIDebug1> dxgiDebug;
        if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
        {
            dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
        }
    }
#endif

}

bool D3D12App::Initialize(const HWND& hwnd)
{
    HRESULT result = S_OK;

    UINT factoryFlag = 0;
#if defined(DEBUG) || defined(_DEBUG)
    ComPtr<ID3D12Debug> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
    {
        debug->EnableDebugLayer();
    }

    ComPtr<IDXGIInfoQueue> dxgiInfoQue;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQue))))
    {
        factoryFlag = DXGI_CREATE_FACTORY_DEBUG;
        
        dxgiInfoQue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
    }
#endif
    result = CreateDXGIFactory2(factoryFlag, IID_PPV_ARGS(&m_dxgi));
    assert(SUCCEEDED(result));

    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_12_1,
        D3D_FEATURE_LEVEL_12_0,
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    // Check all adapters (Graphics cards)
    UINT i = 0;
    ComPtr<IDXGIAdapter> pAdapter;
    std::vector<ComPtr<IDXGIAdapter>> adapterList;
    while (m_dxgi->EnumAdapters(i, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        pAdapter->GetDesc(&desc);
        std::wstring text = L"***Graphic card: ";
        text += desc.Description;
        text += L"/n";

        std::cout << text.c_str() << std::endl;
        adapterList.push_back(std::move(pAdapter));
        ++i;
    }

    for (auto& fLevel : featureLevels)
    {
        //result = D3D12CreateDevice(nullptr, fLevel, IID_PPV_ARGS(m_device.ReleaseAndGetAddressOf()));
        /*-------Use strongest graphics card (adapter) GTX-------*/
        result = D3D12CreateDevice(adapterList[1].Get(), fLevel, IID_PPV_ARGS(m_device.GetAddressOf()));

        if (SUCCEEDED(result))
            break;

        if (FAILED(result)) {
            return false;
        }
    }
   
    CreateCommandFamily();
    CreateFrameResources();

    m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));
    m_targetFenceValue = m_fence->GetCompletedValue();

    m_cmdAlloc->Reset();
    m_cmdList->Reset(m_cmdAlloc.Get(), nullptr);

    CreateSwapChain(hwnd);
    CreateBackBufferView();
    CreateTextureManager();
    CreatePSOManager();
    CreatePipelines();
    CreateWorldPassConstant();
    CreateMaterials();
    CreateShadowDepthBuffer();
    CreateViewDepthBuffer();
    CreatePostEffect();
    CreateDefaultTexture();
    CreatePMDModel();
    CreatePrimitive();
    CreateBlurFilter();
    CreateSprite();
    assert(CreateImGui(hwnd));

    m_cmdList->Close();
    ComPtr<ID3D12CommandList> cmdList;
    m_cmdList->QueryInterface(IID_PPV_ARGS(&cmdList));
    m_cmdQue->ExecuteCommandLists(1, cmdList.GetAddressOf());
    UpdateFence();
    WaitForGPU();

    EffekseerInit();

    m_updateBuffers.Clear();
    m_pmdManager->ClearSubresources();
    m_primitiveManager->ClearSubresources();
    m_spriteMng->ClearSubresources();

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

        // fog
        mappedData->FogColor = XMFLOAT4(0.6f, 0.6f, 0.6f, 1.0f);
        mappedData->FogRange = 150.0f;
        mappedData->FogStart = 10.0f;

        XMVECTOR lightPos = { 0,30,40,1 };
       
        mappedData->Lights[0] = Light();
        mappedData->Lights[1] = Light();
        mappedData->Lights[2] = Light();
        mappedData->Lights[0].Direction = { 0.0f, -1.0f, -1.0f };
        mappedData->Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
        mappedData->Lights[1].Direction = { -1.0f, -1.0f, 0.0f };
        mappedData->Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
        mappedData->Lights[2].Direction = { -1.0f, -1.0f, -1.0f };
        mappedData->Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

        auto lightDirection = XMLoadFloat3(&mappedData->Lights[0].Direction);
        auto lightViewProj = XMMatrixLookToRH(lightPos, lightDirection, { 0,1,0,0 }) *
            XMMatrixOrthographicRH(200.0f, 200.0f, 1.0f, 500.0f);

        XMStoreFloat4x4(&mappedData->Lights[0].ProjectMatrix, lightViewProj);

        gpuAddress += stride_bytes;
    }

    return true;
}

void D3D12App::CreateMaterials()
{
    m_materialCB.Create(m_device.Get(), 4, true);

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
    ++index;

    m_materialIndices["woodCrate"] = index;
    handledMappedData = m_materialCB.GetHandleMappedData(index);
    handledMappedData->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    handledMappedData->FresnelF0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
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
}

void D3D12App::CreatePrimitive()
{
    m_primitiveManager = std::make_unique<PrimitiveManager>();

    auto brickGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["brick0"]);
    auto stoneGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["stone0"]);
    auto tileGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["tile0"]);
    auto woodCrateGpuAdress = m_materialCB.GetGPUVirtualAddress(m_materialIndices["woodCrate"]);

    m_primitiveManager->SetDevice(m_device.Get());
    m_primitiveManager->SetDefaultTexture(m_whiteTexture.Get(), m_blackTexture.Get(), m_gradTexture.Get());
    m_primitiveManager->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress());
    m_primitiveManager->SetWorldShadowMap(m_shadowDepthBuffer.Get());
    //m_primitiveManager->SetViewDepth(m_viewDepthBuffer.Get());
    m_primitiveManager->Create("grid", GeometryGenerator::CreateGrid(200.0f, 100.0f, 30, 40), tileGpuAdress, m_texMng->Get("tile"));
    m_primitiveManager->Create("sphere", GeometryGenerator::CreateSphere(5.0f, 20, 20) , stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere1", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere2", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere3", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere4", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere5", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere6", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere7", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere8", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere9", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere10", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere11", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere12", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("sphere13", GeometryGenerator::CreateSphere(5.0f, 20, 20), stoneGpuAdress, m_texMng->Get("stone"));
    m_primitiveManager->Create("cylinder", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1) , brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder1", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder2", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder3", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder4", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder5", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder6", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder7", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder8", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder9", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder10", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder11", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder12", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("cylinder13", GeometryGenerator::CreateCylinder(3.0f, 5.0f, 20.0f, 20, 1), brickGpuAdress, m_texMng->Get("brick"));
    m_primitiveManager->Create("box", GeometryGenerator::CreateBox(20.0f, 20.0f, 20.0f), woodCrateGpuAdress, m_texMng->Get("wire-fence"));
    //assert(m_primitiveManager->Init(m_cmdList.Get()));
    m_primitiveManager->Init(m_cmdList.Get());

    float startX = 100.0f;
    float startZ = 50.0f - 3.0f;
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
    startZ = -50.0f + 3.0f;
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

    m_primitiveManager->Move("box", 0.0f, 40.0f, 0.0f);
    m_primitiveManager->ScaleTexture("grid", 4.0f, 4.0f);
}

void D3D12App::CreateBlurFilter()
{
    m_blurFilter = std::make_unique<BlurFilter>();
    auto bbDesc = m_backBuffer[m_currentBackBuffer]->GetDesc();
    m_blurFilter->Create(m_device.Get(), bbDesc.Width, bbDesc.Height, bbDesc.Format);
}

void D3D12App::CreateSprite()
{
    m_spriteMng = std::make_unique<SpriteManager>();
    m_spriteMng->SetDevice(m_device.Get());
    m_spriteMng->SetWorldPassConstantGpuAddress(m_worldPCBuffer.GetGPUVirtualAddress(m_currentFrameResourceIndex));
    m_spriteMng->Create("tree0", 50.0f, 50.0f, m_texMng->Get("tree0"));
    m_spriteMng->Create("tree1", 20.0f, 40.0f, m_texMng->Get("tree1"));
    m_spriteMng->Create("tree2", 20.0f, 40.0f, m_texMng->Get("tree2"));
    m_spriteMng->Create("tree3", 20.0f, 40.0f, m_texMng->Get("tree1"));
    m_spriteMng->Create("tree4", 20.0f, 40.0f, m_texMng->Get("tree2"));

    m_spriteMng->Init(m_cmdList.Get());

    m_spriteMng->Move("tree0", 30.0f, 25.0f, -20.0f);
    m_spriteMng->Move("tree1", -30.0f, 20.0f, -30.0f);
    m_spriteMng->Move("tree2", -25.0f, 20.0f, 30.0f);
    m_spriteMng->Move("tree3", 10.0f, 20.0f, 30.0f);
    m_spriteMng->Move("tree4", 50.0f, 20.0f, -10.0f);
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
    UpdateImGui(deltaTime);

    m_currentFrameResourceIndex = (m_currentFrameResourceIndex + 1) % num_frame_resources;
    m_currentFrameResource = &m_frameResources[m_currentFrameResourceIndex];

    // Check if current GPU fence value is processing at current frame resource
    // If current GPU fence value < current frame resource value
    // -> GPU is processing at current frame resource
    auto fenceValue = m_fence->GetCompletedValue();
    if (m_currentFrameResource->FenceValue != 0 && fenceValue < m_currentFrameResource->FenceValue)
    {
        auto fenceEvent = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        m_fence->SetEventOnCompletion(m_currentFrameResource->FenceValue, fenceEvent);
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
        m_rtTex.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtNormalTex.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_rtBrightTex.Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_shadowDepthBuffer.Get(),                  // resource buffer
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,               // state before
        D3D12_RESOURCE_STATE_DEPTH_WRITE);        // state after
    m_cmdList->ResourceBarrier(1, &barrier);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_viewDepthBuffer.Get(),                  // resource buffer
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
    RenderToRenderTargetTextures();
    RenderToBackBuffer();
    RenderImGui();
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
    // Wait GPU finished processing before terminate application
    if (m_device != nullptr)
    {
        UpdateFence();
        WaitForGPU();
    }
    m_currentFrameResource = nullptr;
    EffekseerTerminate();

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
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
