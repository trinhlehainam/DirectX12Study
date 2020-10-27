#include "Application.h"
#include "DirectX12/Dx12Wrapper.h"

namespace
{
	const auto className = L"DX12Study";
	constexpr int WINDOW_WIDTH = 1280;
	constexpr int WINDOW_HEIGHT = 720;
}

Application::Application()
{

}

Application::~Application() = default;

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch(msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

Application& Application::Instance()
{
    static Application app;
    return app;
}

bool Application::Initialize()
{
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
	auto wndHandle = CreateWindow(
		className,					// 上で登録した身分証のクラス名
		L"DirectX12Study",			// ウインドウ実体の名前
		WS_OVERLAPPEDWINDOW,		// 普通のウインドウ
		CW_USEDEFAULT,				// X座標デフォルト
		CW_USEDEFAULT,				// Y座標デフォルト
		WINDOW_WIDTH,				// ウインドウ幅
		WINDOW_HEIGHT,				// ウインドウ高さ
		nullptr,					// 親ウインドウのハンドル
		nullptr,					// メニューのハンドル
		inst_,						// このアプリケーションのインスタント
		nullptr);					// lparam

	if (wndHandle == 0)
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

	dxWrapper_ = std::make_unique<Dx12Wrapper>();
	if (!dxWrapper_->Initialize(wndHandle))
		return false;

	ShowWindow(wndHandle, SW_SHOW);
	UpdateWindow(wndHandle);

	return true;
}

void Application::Run()
{
	MSG msg = {};
	while (true)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		if (msg.message == WM_QUIT)
			break;
		dxWrapper_->Update();
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
