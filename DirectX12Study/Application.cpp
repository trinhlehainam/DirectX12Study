#include "Application.h"
#include "DirectX12/D3D12App.h"
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

		SetWindowText(wndHandle_, windowTitle.c_str());

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
		dxWrapper_->ClearKeyState();
		break;
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		
		dxWrapper_->OnKeyDown(static_cast<uint8_t>(wParam));

		// Bitwise with 30th bit of lParam to find previous key state
		if (lParam & 0x40000000)
		{
			
		}

		break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
		dxWrapper_->OnKeyUp(static_cast<uint8_t>(wParam));
		
		break;
		/**************************************************/

		/*******************MOUSE INPUT********************/
	case WM_MOUSEMOVE:
	{
		const POINTS p = MAKEPOINTS(lParam);
		dxWrapper_->OnMouseMove(static_cast<int>(p.x), static_cast<int>(p.y));
	}
	break;
	case WM_LBUTTONDOWN:
	{
		const POINTS p = MAKEPOINTS(lParam);
		dxWrapper_->OnMouseLeftDown(static_cast<int>(p.x), static_cast<int>(p.y));
	}
	break;
	case WM_RBUTTONDOWN:
	{
		const POINTS p = MAKEPOINTS(lParam);
		dxWrapper_->OnMouseRightDown(static_cast<int>(p.x), static_cast<int>(p.y));
	}
	break;
	case WM_LBUTTONUP:
	{
		const POINTS p = MAKEPOINTS(lParam);
		dxWrapper_->OnMouseLeftUp(static_cast<int>(p.x), static_cast<int>(p.y));
	}
	break;
	case WM_RBUTTONUP:
	{
		const POINTS p = MAKEPOINTS(lParam);
		dxWrapper_->OnMouseRightUp(static_cast<int>(p.x), static_cast<int>(p.y));
	}
	break;
	case WM_MBUTTONDOWN:
		break;
	case WM_MBUTTONUP:
		break;
	case WM_MOUSEWHEEL:
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

	dxWrapper_ = std::make_unique<D3D12App>();
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
		dxWrapper_->Render();
	}
}

void Application::Terminate()
{
	dxWrapper_->Terminate();
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
