#pragma once
#include "Subsystem.h"
#include "WindowConfig.h"

namespace Gleam {

class EventSystem;

class WindowSystem final : public EngineSubsystem
{
	friend class EventSystem;
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown() override;
    
    void Configure(const WindowConfig& config);

	void SetDisplayMode(uint32_t mode) const;

	DisplayMode GetPrimaryDisplayMode() const;
    
	DisplayMode GetCurrentDisplayMode() const;

    DisplayMode GetDisplayMode(uint32_t monitor) const;
    
    TArray<DisplayMode> GetAvailableDisplayModes() const;

	SDL_Window* GetSDLWindow() const
	{
		return mWindow;
	}

private:

	void EventHandler(SDL_WindowEvent windowEvent);

	Engine* mEngine;

	SDL_Window* mWindow;

};

} // namespace Gleam
