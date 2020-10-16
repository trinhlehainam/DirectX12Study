#include <Windows.h>
#include "Application.h"

int WINAPI WinMain(HINSTANCE inst, HINSTANCE prev, LPSTR, int)
{
	auto& app = Application::Instance();
	if (!app.Initialize())
		return -1;
	app.Run();
	app.Terminate();
	return 0;
}