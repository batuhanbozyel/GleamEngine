#pragma once
#include "Gleam.h"
#include "Application.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
Gleam::Application* Gleam::CreateApplication();

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char* argv[])
{
	Gleam::Application* app = Gleam::CreateApplication();

	app->Run();

	delete app;

	return EXIT_SUCCESS;
}
#else
#error Target platform is not currently supported
#endif
