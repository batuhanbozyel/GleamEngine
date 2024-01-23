#pragma once
#include "Subsystem.h"
#include "WindowConfig.h"

namespace Gleam {

class WindowSystem final : public Subsystem
{
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void Configure(const WindowConfig& config);
    
    DisplayMode GetCurrentDisplayMode(uint32_t monitor) const;
    
    TArray<DisplayMode> GetAvailableDisplayModes() const;

	Size GetResolution() const
	{
        return mConfig.size;
	}

	const WindowConfig& GetConfiguration() const
	{
		return mConfig;
	}

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

	void EventHandler(SDL_WindowEvent windowEvent);

private:

	SDL_Window* mWindow;
	WindowConfig mConfig;

};

} // namespace Gleam
