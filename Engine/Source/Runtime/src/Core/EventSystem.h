#pragma once
#include "Subsystem.h"

union SDL_Event;
struct SDL_Window;

namespace Gleam {

using EventHandlerFn = std::function<void(const SDL_Event*)>;

class EventSystem final : public EngineSubsystem
{
public:

	void Update();

	void SetEventHandler(EventHandlerFn&& fn);

private:

	static int SDLCALL SDL_EventHandler(void* data, SDL_Event* e);

	EventHandlerFn mEventHandler;

};

} // namespace Gleam
