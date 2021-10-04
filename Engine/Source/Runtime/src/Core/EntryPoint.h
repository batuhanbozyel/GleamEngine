#pragma once
#include "PlatformTargetDefines.h"

#ifdef __cplusplus
extern "C"
#endif

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS)
Gleam::Application * Gleam::CreateApplication();

int main(int argc, char* argv[])
{
	Gleam::Application* app = Gleam::CreateApplication();

    app->Run();

	delete app;

	return 0;
}
#else
#error Target platform is not currently supported
#endif
