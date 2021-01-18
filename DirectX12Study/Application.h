#pragma once
#include <Windows.h>
#include <memory>

#include "common.h"
#include "Common/Timer.h"

class D3D12App;

/// <summary>
/// Application management
/// Singleton
/// </summary>
class Application
{
public:
	static Application& Instance();
	bool Initialize();
	void Run();
	void Terminate();
public:
	Size GetWindowSize() const;
	float GetAspectRatio() const;
	void SetWindowSize(const size_t& width, const size_t& height);
private:
	static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
	LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
private:
	Application();
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;
	~Application();
private:

	HINSTANCE m_inst;
	HWND m_hWnd;
	std::unique_ptr<D3D12App> m_d3d12app;
	
	size_t clientWidth_ = 800;
	size_t clientHeight_ = 600;

private:
	bool isRunning_;
	Timer m_timer;
	// Compute average frames per seconds, and also average time( milliseconds ) it takes to render one frame
	// The information of theses values are written in window caption bar (window caption)
	void CalculatePerformance();
};

