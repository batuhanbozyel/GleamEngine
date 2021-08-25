#pragma once
#include "PlatformTargetDefines.h"

namespace Gleam {

class GLEAM_API Application
{
public:

	void Run();

private:

};

}

#ifdef PLATFORM_WINDOWS
#include <windows.h>

#define GLEAM_MAIN(appPtr)\
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR Cmd, int showCmd)\
{\
	Gleam::Application* app = static_cast<Gleam::Application*>(appPtr);\
	app->Run();\
	delete app;\
}
#endif