#pragma once
#include <Windows.h>
#include <memory>
/// <summary>
/// Application management
/// Singleton
/// </summary>

class Dx12Wrapper;

class Application
{
private:
	HINSTANCE inst_;
	HWND windowHandle_;
	std::unique_ptr<Dx12Wrapper> dx_;

	Application() = default;
	Application(const Application&) = delete;
	void operator = (const Application&) = delete;
	~Application() = default;
public:
	static Application& Instance();
	bool Initialize();
	void Run();
	void Terminate();
};

