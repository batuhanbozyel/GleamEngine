#pragma once
#include "Window.h"

#ifdef __cplusplus
extern "C"
#endif

namespace Gleam {

class Application
{
public:

	explicit Application(const WindowProperties& props, WindowFlag flag);


	void Run();

private:

	bool m_Running = true;
	UniquePtr<Window> m_Window = nullptr;

};

Application* CreateApplication();
}