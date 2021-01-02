#include "Application.h"
#include "Graphics/D3D12App.h"
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
	SetWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
}

Application::~Application()
{

}

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

		SetWindowText(m_hWnd, windowTitle.c_str());

		frameCnt = 0;
		elapsedTime += 1.0f;
	}
	
}

LRESULT CALLBACK Application::WindowProcedure(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	auto& app = Application::Instance();
	return app.ProcessMessage(hwnd, msg, wparam, lparam);
}

LRESULT Application::ProcessMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		/*******************KEY BOARD*******************/
	case WM_KILLFOCUS:
		d3d12app->ClearKeyState();
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_CHAR:
		d3d12app->OnWindowsKeyboardMessage(Msg, wParam, lParam);
		break;
		/**************************************************/

		/*******************MOUSE INPUT********************/
	case WM_MOUSEMOVE:
		d3d12app->OnWindowsMouseMessage(Msg, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_MBUTTONDOWN:
		d3d12app->OnWindowsMouseMessage(Msg, wParam, lParam);
		::SetCapture(m_hWnd);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	case WM_MBUTTONUP:
		d3d12app->OnWindowsMouseMessage(Msg, wParam, lParam);
		::ReleaseCapture();
		break;
	case WM_MOUSEWHEEL:
		d3d12app->OnWindowsMouseMessage(Msg, wParam, lParam);
		break;
		/**************************************************/
	default:
		return DefWindowProc(hWnd, Msg, wParam, lParam);
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
	m_hWnd = CreateWindow(
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

	if (m_hWnd == 0)
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

	ShowWindow(m_hWnd, SW_SHOW);
	UpdateWindow(m_hWnd);

	d3d12app = std::make_unique<D3D12App>();
	if (!d3d12app->Initialize(m_hWnd))
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
		d3d12app->ProcessMessage();
		d3d12app->Update(timer_.DeltaTime());
		d3d12app->Render();
	}
}

void Application::Terminate()
{
	d3d12app->Terminate();
	UnregisterClassW(className, inst_);
}

Size Application::GetWindowSize() const
{
	return { clientWidth_,clientHeight_ };
}

float Application::GetAspectRatio() const
{
	return static_cast<float>(clientWidth_)/clientHeight_;
}

void Application::SetWindowSize(const size_t& width, const size_t& height)
{
	clientHeight_ = height;
	clientWidth_ = width;
}
