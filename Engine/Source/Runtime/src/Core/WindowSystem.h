#pragma once
#include "Subsystem.h"
#include "WindowConfig.h"

struct SDL_WindowEvent;
struct SDL_Window;

namespace Gleam {

class EventSystem;

class WindowSystem final : public EngineSubsystem
{
	friend class EventSystem;
public:
    
    virtual void Initialize(Engine* engine) override;
    
    virtual void Shutdown(Engine* engine) override;
    
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

	float GetDisplayScale() const;
	
	Size GetResolution() const;

private:

	void ApplyConfig(const WindowConfig& config);

	void EventHandler(SDL_WindowEvent windowEvent);

	Engine* mEngine;

	SDL_Window* mWindow;

};

} // namespace Gleam
