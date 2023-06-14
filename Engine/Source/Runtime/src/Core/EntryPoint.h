#pragma once
#include "Gleam.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
Gleam::Application* Gleam::CreateApplicationInstance();

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char* argv[])
{
	Gleam::Application* game = Gleam::CreateApplicationInstance();

    game->Run();

	delete game;

	return EXIT_SUCCESS;
}
#else
#error Target platform is not currently supported
#endif
