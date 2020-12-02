#include "Application.h"
#include "DirectX12/Dx12Wrapper.h"
#include <string>

namespace
{
	const auto className = L"DX12Study";
	const auto window_caption = L"DirectX12Study";
	constexpr int WINDOW_WIDTH = 1280;
	constexpr int WINDOW_HEIGHT = 720;
}

Application::Application()
{

}

Application::~Application() = default;

void Application::CalculatePerformance()
{
	// Compute average frames per seconds, and also average time( milliseconds ) it takes to render one frame
	// The information of theses values are written in window caption bar (window title)

	static int frameCnt = 0;
	static float elapsedTime = 0.0f;
	++frameCnt;

	if (timer_.TotalTime() - elapsedTime >= 1.0f)
	{
		float fps = static_cast<float>(frameCnt);
		float mspf = second_to_millisecond / fps;

		std::wstring fpsStr = L"    fps: " + std::to_wstring(fps);
		std::wstring mspfStr = L"    mspf: " + std::to_wstring(mspf);

		std::wstring windowTitle = window_caption + fpsStr + mspfStr;

		SetWindowText(wndHandle_, windowTitle.c_str());

		frameCnt = 0;
		elapsedTime += 1.0f;
	}
	
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

Application& Application::Instance()
{
    static Application app;
    return app;
}

bool Application::Initialize()
{
	isRunning_ = true;

	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;;
	wc.hInstance = inst_;
	wc.lpszClassName = className;
	wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
	
	wc.lpfnWndProc = WindowProcedure;
	auto regCls = RegisterClassEx(&wc);
	RECT rc = { 0,0,WINDOW_WIDTH, WINDOW_HEIGHT };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);
	wndHandle_ = CreateWindow(
		className,					// 上で登録した身分証のクラス名
		window_caption,			// ウインドウ実体の名前
		WS_OVERLAPPEDWINDOW,		// 普通のウインドウ
		CW_USEDEFAULT,				// X座標デフォルト
		CW_USEDEFAULT,				// Y座標デフォルト
		WINDOW_WIDTH,				// ウインドウ幅
		WINDOW_HEIGHT,				// ウインドウ高さ
		nullptr,					// 親ウインドウのハンドル
		nullptr,					// メニューのハンドル
		inst_,						// このアプリケーションのインスタント
		nullptr);					// lparam

	if (wndHandle_ == 0)
	{
		LPVOID messageBuffer = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&messageBuffer,
			0,
			nullptr);
		OutputDebugString((LPWSTR)messageBuffer);
		return false;
	}

	ShowWindow(wndHandle_, SW_SHOW);
	UpdateWindow(wndHandle_);

	dxWrapper_ = std::make_unique<Dx12Wrapper>();
	if (!dxWrapper_->Initialize(wndHandle_))
		return false;

	return true;
}

void Application::Run()
{
	MSG msg = {};

	timer_.Reset();

	while (isRunning_)
	{
		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				isRunning_ = false;
				break;
			}
		}
		if (!isRunning_)
			break;
		timer_.Tick();
		CalculatePerformance();
		dxWrapper_->Update(timer_.DeltaTime());
	}
}

void Application::Terminate()
{
	dxWrapper_->Terminate();
	UnregisterClassW(className, inst_);
}

Size Application::GetWindowSize() const
{
	return { WINDOW_WIDTH,WINDOW_HEIGHT };
}
