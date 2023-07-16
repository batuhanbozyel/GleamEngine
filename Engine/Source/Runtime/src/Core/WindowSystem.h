#pragma once
#include "Subsystem.h"
#include "WindowConfig.h"

namespace Gleam {

class WindowSystem final : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void ConfigureWindow(const WindowProperties& props);

	Size GetResolution() const
	{
		return { static_cast<float>(mProperties.display.width), static_cast<float>(mProperties.display.height) };
	}

	const WindowProperties& GetProperties() const
	{
		return mProperties;
	}

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

	void EventHandler(SDL_WindowEvent windowEvent);

private:

	SDL_Window* mWindow;
	WindowProperties mProperties;

};

} // namespace Gleam
