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
	auto wndHandle = CreateWindow(
		className,					// ��œo�^�����g���؂̃N���X��
		L"DirectX12Study",			// �E�C���h�E���̖̂��O
		WS_OVERLAPPEDWINDOW,		// ���ʂ̃E�C���h�E
		CW_USEDEFAULT,				// X���W�f�t�H���g
		CW_USEDEFAULT,				// Y���W�f�t�H���g
		WINDOW_WIDTH,				// �E�C���h�E��
		WINDOW_HEIGHT,				// �E�C���h�E����
		nullptr,					// �e�E�C���h�E�̃n���h��
		nullptr,					// ���j���[�̃n���h��
		inst_,						// ���̃A�v���P�[�V�����̃C���X�^���g
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

	ShowWindow(wndHandle, SW_SHOW);
	UpdateWindow(wndHandle);

	dxWrapper_ = std::make_unique<Dx12Wrapper>();
	if (!dxWrapper_->Initialize(wndHandle))
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
