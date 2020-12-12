#pragma once
#include <unordered_map>
#include <string>
#include <d3dx12.h>

#include "PMDMeshes.h"

using Microsoft::WRL::ComPtr;

class PMDModel;

class PMDManager
{
public:
	PMDManager();
	PMDManager(ID3D12Device* pDevice);
	~PMDManager();

	//
	/*----Functions use for Initialize PMD Manager------*/
	//

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
public:
	bool Add(const std::string& name);
	PMDModel& Get(const std::string& name);

	void Render();
	void Update();
private:
	void Sleep();
	void PrivateRender();
	void PrivateUpdate();

	using Func_ptr = void (PMDManager::*)();
	Func_ptr m_updateFunc;
	Func_ptr m_renderFunc;

	bool m_isInitDone = false;
private:
	bool CreatePipeline();
	bool CreateRootSignature();
	bool CreatePipelineStateObject();
	// check default buffers are initialized or not
	bool CheckDefaultBuffers();

	/*----------RESOURCE FROM ENGINE----------*/
	// Device from engine
	ID3D12Device* m_device = nullptr;

	// Default resource from engine
	ID3D12Resource* m_whiteTexture = nullptr;
	ID3D12Resource* m_blackTexture = nullptr;
	ID3D12Resource* m_gradTexture = nullptr;
	/*-----------------------------------------*/

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
	PMDMeshes m_meshes;
};

