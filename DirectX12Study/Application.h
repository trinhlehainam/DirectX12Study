#pragma once
#include <Windows.h>
#include <memory>

#include "common.h"
#include "Common/Timer.h"

class Dx12Wrapper;

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

	Size GetWindowSize() const;
	float GetAspectRatio() const;
	void SetWindowSize(const size_t& width, const size_t& height);
private:
	HINSTANCE inst_;
	HWND wndHandle_;
	std::unique_ptr<Dx12Wrapper> dxWrapper_;
	bool isRunning_;
	Timer timer_;

	size_t clientWidth_ = 800;
	size_t clientHeight_ = 600;

	Application();
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;
	~Application();

	// Compute average frames per seconds, and also average time( milliseconds ) it takes to render one frame
	// The information of theses values are written in window caption bar (window caption)
	void CalculatePerformance();
};

