#pragma once
#include "Gleam.h"
#include "Game.h"

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
Gleam::Game* Gleam::CreateGameInstance();

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char* argv[])
{
	Gleam::Game* game = Gleam::CreateGameInstance();

    game->Run();

	delete game;

	return EXIT_SUCCESS;
}
#else
#error Target platform is not currently supported
#endif
