#pragma once
#include <Windows.h>
#include <memory>

#include "common.h"

class Dx12Wrapper;

/// <summary>
/// Application management
/// Singleton
/// </summary>
class Application
{
private:
	HINSTANCE inst_;
	HWND windowHandle_;
	std::unique_ptr<Dx12Wrapper> dxWrapper_;
	bool isRunning_;

	Application();
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;
	~Application();
public:
	static Application& Instance();
	bool Initialize();
	void Run();
	void Terminate();
	Size GetWindowSize() const;
};

