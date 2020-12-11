#pragma once
#include <unordered_map>
#include <string>
#include <d3dx12.h>

using Microsoft::WRL::ComPtr;

class PMDModel;

class PMDManger
{
public:
	PMDManger();
	PMDManger(ID3D12Device* pDevice);
	~PMDManger();

	bool SetDevice(ID3D12Device* pDevice);
	bool SetWorldPassConstant(ID3D12Resource* pWorldPassConstant, size_t bufferSize);
	bool SetWorldShadowMap(ID3D12Resource* pShadowDepthBuffer);
	// The order is 
	// 1: WHITE TEXTURE
	// 2: BLACK TEXTURE
	// 3: GRADIENT TEXTURE
	bool SetDefaultBuffer(ID3D12Resource* pWhiteTexture, ID3D12Resource* pBlackTexture,
		ID3D12Resource* pGradTexture);
	bool Init();
	bool Add(const std::string& name);
	bool Get(const std::string& name);
	void Update();
	void Render();
private:
	bool CreatePipeline();
	bool CreateRootSignature();
	bool CreatePipelineStateObject();
private:
	// Device from engine
	ID3D12Device* m_device = nullptr;

	// Default resource from engine
	ID3D12Resource* m_whiteTexture = nullptr;
	ID3D12Resource* m_blackTexture = nullptr;
	ID3D12Resource* m_gradTexture = nullptr;
private:
	ComPtr<ID3D12RootSignature> m_rootSig = nullptr;
	ComPtr<ID3D12PipelineState> m_pipeline = nullptr;

	// World pass constant buffer binds only to VERTEX BUFFER
	// Descriptor heap stores descriptor of world pass constant buffer
	// Use for binding resource of engine to this pipeline
	ComPtr<ID3D12DescriptorHeap> m_worldPCBHeap = nullptr;
	
	// Shadow depth buffer binds only to PIXEL SHADER
	// Descriptor heap stores descriptor of shadow depth buffer
	// Use for binding resource of engine to this pipeline
	ComPtr<ID3D12DescriptorHeap> m_shadowDepthHeap = nullptr;
private:
	std::unordered_map<std::string, PMDModel> m_models;

};

