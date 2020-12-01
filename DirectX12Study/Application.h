#pragma once
#include <Windows.h>
#include <memory>

#include "common.h"
#include "Common/GameTimer.h"

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
private:
	HINSTANCE inst_;
	HWND windowHandle_;
	std::unique_ptr<Dx12Wrapper> dxWrapper_;
	bool isRunning_;
	GameTimer timer_;

	Application();
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;
	~Application();
};

