#include "gpch.h"
#include "Window.h"

using namespace Gleam;

Window::Window(const WindowProperties& props, WindowFlag flag)
	: m_Props(props)
{
	int initSucess = SDL_Init(SDL_INIT_VIDEO);
	ASSERT(initSucess == 0, "Window subsystem initialization failed!");

	m_Window = SDL_CreateWindow(props.Title.c_str(),
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		props.Width, props.Height,
		static_cast<uint32_t>(flag));

	ASSERT(m_Window, "Window creation failed!");
}

Window::~Window()
{
	SDL_DestroyWindow(m_Window);
	SDL_Quit();
}

void Window::OnUpdate()
{
	while(SDL_PollEvent(&m_Event));
}

void Window::OnResize(uint32_t width, uint32_t height)
{

}
