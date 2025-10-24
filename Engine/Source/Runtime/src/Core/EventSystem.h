#pragma once
#include "Subsystem.h"

#include <functional>
#include <SDL3/SDL.h>

namespace Gleam {

using EventHandlerFn = std::function<bool(const SDL_Event*)>;

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
