#pragma once
#include "Gleam.h"
#include "Application.h"

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

	return EXIT_SUCCESS;
}
#else
#error Target platform is not currently supported
#endif
