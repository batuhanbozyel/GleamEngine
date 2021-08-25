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

#if defined(PLATFORM_WINDOWS)
#include <windows.h>

#define GLEAM_MAIN(appPtr)\
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR Cmd, int showCmd)\
{\
	Gleam::Application* app = static_cast<Gleam::Application*>(appPtr);\
	app->Run();\
	delete app;\
}

#elif defined(PLATFORM_MACOS)

#define GLEAM_MAIN(appPtr)\
int main(int argc, const char* argv[])\
{\
    Gleam::Application* app = static_cast<Gleam::Application*>(appPtr);\
    app->Run();\
    delete app;\
}\

#endif
