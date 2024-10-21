#pragma once
#include "Gleam.h"
#include <SDL3/SDL_main.h>

#if defined(PLATFORM_WINDOWS) || defined(PLATFORM_MACOS) || defined(PLATFORM_IOS)
Gleam::Application* Gleam::CreateApplicationInstance(const Gleam::CommandLine& cli);

#ifdef USE_DIRECTX_RENDERER
#include <d3d12.h>

extern "C"
{
	__declspec(dllexport) extern const UINT D3D12SDKVersion = D3D12_SDK_VERSION;
	__declspec(dllexport) extern LPCSTR D3D12SDKPath = ".\\D3D12\\";
}
#endif

int main(int argc, char* argv[])
{
    Gleam::Engine engine;
    Gleam::Globals::Engine = &engine;
    engine.Initialize();

	// TODO: parse command line, check if project is provided, then launch Game, otherwise launch custom application instance
	Gleam::Globals::GameInstance = Gleam::CreateApplicationInstance(Gleam::CommandLine(argc, argv));
	Gleam::Globals::GameInstance->Run();

	delete Gleam::Globals::GameInstance;
    engine.Shutdown();
	return EXIT_SUCCESS;
}
#else
#error Target platform is not currently supported
#endif
