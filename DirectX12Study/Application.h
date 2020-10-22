#pragma once
#include <Windows.h>
#include <memory>

struct Size
{
	size_t width;
	size_t height;
};

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

