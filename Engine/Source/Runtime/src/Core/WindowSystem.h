#pragma once
#include "Subsystem.h"
#include "WindowConfig.h"

namespace Gleam {

class EventSystem;

class WindowSystem final : public Subsystem
{
	friend class EventSystem;
public:
    
    virtual void Initialize() override;
    
    virtual void Shutdown() override;
    
    void Configure(const WindowConfig& config);
    
    DisplayMode GetCurrentDisplayMode(uint32_t monitor) const;
    
    TArray<DisplayMode> GetAvailableDisplayModes() const;

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

private:

	void EventHandler(SDL_WindowEvent windowEvent);

	SDL_Window* mWindow;

};

} // namespace Gleam
