#pragma once
#ifndef __GLEAM_REFLECTION__
#include <Runtime.Reflection.generated.h>
#endif

#include "Application.h"
#include "Engine.h"
#include "Globals.h"

#include <SDL3/SDL_main.h>
#include <Reflection/Database.h>

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

// For EASTL
void* operator new[](size_t size, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

void* operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* pName, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

int main(int argc, char* argv[])
{
	Gleam::CommandLine cli;
	cli.Parse(argc, argv);
	Gleam::Globals::CLI = &cli;

	Gleam::Reflection::Database reflection;
	reflection.Initialize(Gleam::Filesystem::WorkingDirectory() / "Runtime.Reflection.db");

    Gleam::Engine engine;
    Gleam::Globals::Engine = &engine;
    engine.Initialize(cli);

	// TODO: parse command line, check if project is provided, then launch Game, otherwise launch custom application instance
	Gleam::Globals::GameInstance = Gleam::CreateApplicationInstance(cli);
	Gleam::Globals::GameInstance->Run();

	delete Gleam::Globals::GameInstance;
    engine.Shutdown();
	reflection.Shutdown();
	return EXIT_SUCCESS;
}
#else
#error Target platform is not currently supported
#endif
