#pragma once
#include <string>
#include <memory>
#include <d3d12.h>

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
	bool SetViewDepth(ID3D12Resource* pViewDepthBuffer);
	// The order is 
	// 1: WHITE TEXTURE
	// 2: BLACK TEXTURE
	// 3: GRADIENT TEXTURE
	bool SetDefaultBuffer(ID3D12Resource* pWhiteTexture, ID3D12Resource* pBlackTexture,
		ID3D12Resource* pGradTexture);

	// Need to set up all resource for PMD Manager BEFORE initialize it
	bool Init(ID3D12GraphicsCommandList* cmdList);

	// Use for check PMD Manager is initialized
	// If PMD Manager isn't initialized, some feature of it won't work right
	bool IsInitialized();

	/// <param name="modelName: ">name of model</param>
	/// <param name="modelFilePath: ">path to model's file</param>
	/// <returns>
	/// <para>False if given name of model is already created.</para>
	/// Or path to model file is unvalid
	/// </returns>
	bool CreateModel(const std::string& modelName, const char* modelFilePath);
	bool CreateAnimation(const std::string& animationName, const char* animationFilePath);

	/// <summary>
	/// <para>Clear all resources that updated to default buffer.</para>
	/// <para>Call AFTER GPU updated subresources to default buffer.</para>
	/// </summary>
	bool ClearSubresources();
public:
	void Update(const float& deltaTime);
	void Render(ID3D12GraphicsCommandList* cmdList);

	// Function use for taking models depth value
	// This function DON'T set up ITS own PIPELINE
	// or any pipeline
	// client must set pipeline before use this
	void RenderDepth(ID3D12GraphicsCommandList* cmdList);

	/// <summary>
	/// Play animation
	/// <para>Need to use after PMDManager is initialized</para>
	/// </summary>
	/// <param name="modelName: ">name of model</param>
	/// <param name="animationName: ">name of animation</param>
	/// <returns>
	/// <para> FALSE: if the name in one of the two variable is unvalid </para>
	/// <para> or miss order between model and animation </para>
	/// </returns>
	bool Play(const std::string& modelName, const std::string& animationName);

	// Move models
	bool Move(const std::string& modelName, float moveX, float moveY, float moveZ);
	// Rotate Model
	bool RotateX(const std::string& modelName, float angle);
	bool RotateY(const std::string& modelName, float angle);
	bool RotateZ(const std::string& modelName, float angle);
	// Scale model
	bool Scale(const std::string& modelName, float scaleX, float scaleY, float scaleZ);
private:
	class Impl;
	std::unique_ptr<Impl> m_impl;
};

