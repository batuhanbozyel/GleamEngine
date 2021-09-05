#include "gpch.h"
#include "Window.h"

using namespace Gleam;

Window::Window(const WindowProperties& props, WindowFlag flag)
	: m_Props(props)
{
	SDL_Init(SDL_INIT_VIDEO);

	/*	TODO:
	*	assert initialization
	*/ 

	m_Window = SDL_CreateWindow(props.Title.Raw(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		props.Width, props.Height,
		static_cast<uint32_t>(flag));

	/*  TODO:
	*	assert window creation
	*/
}

Window::~Window()
{
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void Window::OnUpdate()
{
	SDL_PollEvent(&m_Event);
}
