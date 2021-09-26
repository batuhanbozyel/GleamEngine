#include "gpch.h"
#include "Application.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

Application::Application(const WindowProperties& props, WindowFlag flag)
{
	Log::Init();

	m_Window = CreateUnique<Window>(props, flag);
	m_Window->SetEventCallback([this](Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e)
		{
			m_Running = false;
			return true;
		});

		dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e)
		{
			m_Window->OnResize(e.GetWidth(), e.GetHeight());
			return true;
		});
	});
}

void Application::Run()
{
	while (m_Running)
	{
		m_Window->OnUpdate();
	}
}
