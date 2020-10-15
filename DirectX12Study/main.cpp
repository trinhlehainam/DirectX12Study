#include <Windows.h>
#include "Application.h"

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR, int)
{
	auto& app = Application::Instance();
	app.Initialize();
	app.Run();
	app.Terminate();
	return 0;
}