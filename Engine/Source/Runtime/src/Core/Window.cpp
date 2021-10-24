#include "gpch.h"
#include "Window.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

int SDLCALL SDL2_EventCallback(void* data, SDL_Event* e)
{
	switch (e->type)
	{
		case SDL_QUIT:
		{
			EventDispatcher<WindowCloseEvent>::Publish(WindowCloseEvent());
			break;
		}
	}
	return 0;
}

Window::Window(const WindowProperties& props)
	: m_Props(props)
{
	int initSucess = SDL_Init(SDL_INIT_VIDEO);
	ASSERT(initSucess == 0, "Window subsystem initialization failed!");

	m_Window = SDL_CreateWindow(props.Title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		props.Width, props.Height,
		static_cast<uint32_t>(props.Flag));

	ASSERT(m_Window, "Window creation failed!");

	SDL_SetEventFilter(SDL2_EventCallback, nullptr);
}

Window::~Window()
{
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void Window::OnUpdate()
{
	while (SDL_PollEvent(&m_Event));
}

void Window::OnResize(uint32_t width, uint32_t height)
{
	m_Props.Width = width;
	m_Props.Height = height;
	SDL_SetWindowSize(m_Window, width, height);
}
