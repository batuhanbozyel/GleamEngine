#include "gpch.h"
#include "Application.h"

#include "Events/WindowEvent.h"

using namespace Gleam;

Application::Application(const WindowProperties& props)
{
	Log::Init();

	m_Window = CreateUnique<Window>(props);

	EventDispatcher::Subscribe<WindowCloseEvent>([this](const SharedPtr<WindowCloseEvent>& e)
	{
		m_Running = false;
	});
}

void Application::Run()
{
	while (m_Running)
	{
		m_Window->OnUpdate();
	}
}
