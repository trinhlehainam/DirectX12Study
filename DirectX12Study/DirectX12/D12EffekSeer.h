#pragma once
#include <Effekseer/Effekseer.h>
#include <EffekseerRendererDX12/EffekseerRendererDX12.h>
#include <d3dx12.h>

using Microsoft::WRL::ComPtr;

class D12EffekSeer
{
public:
	void Init();
	void Update();
	void Render();
private:
	ComPtr<ID3D12Device> m_device = nullptr;
	ComPtr<ID3D12CommandQueue> m_cmdQue = nullptr;

	EffekseerRenderer::Renderer* m_effekRenderer = nullptr;
	EffekseerRenderer::SingleFrameMemoryPool* m_effekPool = nullptr;
	Effekseer::Manager* m_effekManager = nullptr;
	EffekseerRenderer::CommandList* m_effekCmdList = nullptr;
};

