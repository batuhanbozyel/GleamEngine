#include "gpch.h"
#include "Application.h"

using namespace Gleam;

Application::Application(const WindowProperties& props, WindowFlag flag)
	: m_Window(props, flag)
{

}

void Application::Run()
{
	while (m_Running)
	{
		m_Window.OnUpdate();
	}
}
