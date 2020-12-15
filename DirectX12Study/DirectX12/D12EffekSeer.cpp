#include "D12EffekSeer.h"
#include "../Application.h"

using namespace DirectX;

void D12EffekSeer::Init()
{
    // Create renderer
    auto backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_effekRenderer = EffekseerRendererDX12::Create(m_device.Get(), m_cmdQue.Get(), 2, &backBufferFormat, 1,
        DXGI_FORMAT_UNKNOWN, false, 8000);

    // Create memory pool
    m_effekPool = EffekseerRendererDX12::CreateSingleFrameMemoryPool(m_effekRenderer);

    // Create commandList
    EffekseerRenderer::CommandList* cmdList = EffekseerRendererDX12::CreateCommandList(m_effekRenderer, m_effekPool);

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

    auto& app = Application::Instance();
    auto screenSize = app.GetWindowSize();

    auto viewPos = Effekseer::Vector3D(10.0f, 10.0f, 10.0f);

    m_effekRenderer->SetProjectionMatrix(Effekseer::Matrix44().PerspectiveFovRH(XM_PIDIV4, app.GetAspectRatio(), 1.0f, 500.0f));

    m_effekRenderer->SetCameraMatrix(
        Effekseer::Matrix44().LookAtRH(viewPos, Effekseer::Vector3D(0.0f, 0.0f, 0.0f), Effekseer::Vector3D(0.0f, 1.0f, 0.0f)));
}

void D12EffekSeer::Update()
{
}

void D12EffekSeer::Render()
{
}
